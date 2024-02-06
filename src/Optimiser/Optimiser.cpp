
#include "Optimiser.h"

Optimiser::Optimiser(std::shared_ptr<ModelTranslator> _modelTranslator,
                     std::shared_ptr<PhysicsSimulator> _physicsSimulator,
                     std::shared_ptr<FileHandler> _yamlReader,
                     std::shared_ptr<Differentiator> _differentiator){
    activeModelTranslator = _modelTranslator;
    activePhysicsSimulator = _physicsSimulator;
    activeYamlReader = _yamlReader;
    activeDifferentiator = _differentiator;

    dof = activeModelTranslator->dof;
    num_ctrl = activeModelTranslator->num_ctrl;

//    keypoint_generator = std::make_shared<KeypointGenerator>(activeDifferentiator,
//                                                             activePhysicsSimulator,
//                                                             activeModelTranslator);
    keypoint_generator = KeypointGenerator(activeDifferentiator,
                                           activePhysicsSimulator);

    keypoint_generator->SetCurrentKeypointMethod(activeYamlReader->file_keypoint_method);

}

bool Optimiser::CheckForConvergence(double old_cost, double new_cost){
    double costGrad = (old_cost - new_cost) / new_cost;

    if(costGrad < epsConverge){
        return true;
    }
    return false;
}

void Optimiser::SetTrajecNumber(int trajec_number) {
    currentTrajecNumber = trajec_number;
}

void Optimiser::ReturnOptimisationData(double &_optTime, double &_costReduction, double &_avgPercentageDerivs, double &_avgTimeGettingDerivs, int &_numIterations){

    _optTime = opt_time_ms;
    _costReduction = costReduction;
    _avgPercentageDerivs = avg_percent_derivs;
    _avgTimeGettingDerivs = avg_time_get_derivs_ms;
    _numIterations = numIterationsForConvergence;
}

void Optimiser::GenerateDerivatives(){
    // STEP 1 - Linearise dynamics and calculate first + second order cost derivatives for current trajectory
    // generate the dynamics evaluation waypoints
//    std::cout << "before gen keypoints \n";
    std::vector<std::vector<int>> keyPoints = keypoint_generator->GenerateKeyPoints(horizonLength, X_old, U_old, A, B);

    // Calculate derivatives via finite differencing / analytically for cost functions if available
    if(keypoint_generator.current_keypoint_method.name != "iterative_error"){
        auto start_fd_time = high_resolution_clock::now();
        ComputeDerivativesAtSpecifiedIndices(keyPoints);
        auto stop_fd_time = high_resolution_clock::now();
        auto duration_fd_time = duration_cast<microseconds>(stop_fd_time - start_fd_time);
    }
    else{
        ComputeCostDerivatives();
    }

    InterpolateDerivatives(keyPoints, activeYamlReader->costDerivsFD);

    int totalNumColumnsDerivs = 0;
    for(int i = 0; i < keyPoints.size(); i++){
        totalNumColumnsDerivs += keyPoints[i].size();
    }

    double percentDerivsCalculated = ((double) totalNumColumnsDerivs / (double)numberOfTotalDerivs) * 100.0f;
    percentage_derivs_per_iteration.push_back(percentDerivsCalculated);
    if(verbose_output){
        cout << "percentage of derivs calculated: " << percentDerivsCalculated << endl;
    }

    if(filteringMethod != "none"){
        FilterDynamicsMatrices();
    }
}

void Optimiser::ComputeCostDerivatives(){
    #pragma omp parallel for
    for(int i = 0; i < horizonLength; i++){
        activeModelTranslator->CostDerivatives(i, l_x[i], l_xx[i], l_u[i], l_uu[i], false);
    }

    activeModelTranslator->CostDerivatives(horizonLength - 1,
                                           l_x[horizonLength - 1], l_xx[horizonLength - 1], l_u[horizonLength - 1], l_uu[horizonLength - 1], true);
}

void Optimiser::ComputeDerivativesAtSpecifiedIndices(std::vector<std::vector<int>> keyPoints){

    activePhysicsSimulator->initModelForFiniteDifferencing();

    std::vector<int> timeIndices;
    for(int i = 0; i < keyPoints.size(); i++){
        if(keyPoints[i].size() != 0){
            timeIndices.push_back(i);
        }
    }

    // Loop through keypoints and delete any entries that have no keypoints
    for(int i = 0; i < keyPoints.size(); i++){
        if(keyPoints[i].size() == 0){
            keyPoints.erase(keyPoints.begin() + i);
            i--;
        }
    }

    current_iteration = 0;
    num_threads_iterations = keyPoints.size();
    timeIndicesGlobal = timeIndices;

    // TODO - remove this? It is used for WorkerComputeDerivatives function to compute derivatives in parallel
    keypointsGlobal = keyPoints;

    // Setup all the required tasks
    for (int i = 0; i < keyPoints.size(); ++i) {
        tasks.push_back(&Differentiator::getDerivatives);
    }

    // Get the number of threads available
    const int num_threads = std::thread::hardware_concurrency();  // Get the number of available CPU cores
    std::vector<std::thread> thread_pool;
    for (int i = 0; i < num_threads; ++i) {
        thread_pool.push_back(std::thread(&Optimiser::WorkerComputeDerivatives, this, i));
    }

    for (std::thread& thread : thread_pool) {
        thread.join();
    }
      
    activePhysicsSimulator->resetModelAfterFiniteDifferencing();

    auto time_cost_start = std::chrono::high_resolution_clock::now();

    if(!activeYamlReader->costDerivsFD){
        for(int i = 0; i < horizonLength; i++){
            if(i == 0){
                activeModelTranslator->CostDerivatives(i, l_x[i], l_xx[i], l_u[i], l_uu[i], false);
            }
            else{
                activeModelTranslator->CostDerivatives(i, l_x[i], l_xx[i], l_u[i], l_uu[i], false);
            }
        }
        activeModelTranslator->CostDerivatives(horizonLength - 1,
                                               l_x[horizonLength - 1], l_xx[horizonLength - 1], l_u[horizonLength - 1], l_uu[horizonLength - 1], true);
    }
}

void Optimiser::WorkerComputeDerivatives(int threadId) {
    while (true) {
        int iteration = current_iteration.fetch_add(1);
        if (iteration >= num_threads_iterations) {
            break;  // All iterations done
        }

        int timeIndex = timeIndicesGlobal[iteration];
        bool terminal = false;
        if(timeIndex == horizonLength - 1){
            terminal = true;
        }

        std::vector<int> keyPoints;
        (activeDifferentiator.get()->*(tasks[iteration]))(A[timeIndex], B[timeIndex],
                                        keypointsGlobal[iteration], l_x[timeIndex], l_u[timeIndex], l_xx[timeIndex], l_uu[timeIndex],
                                        activeYamlReader->costDerivsFD, timeIndex, terminal, threadId);
    }
}

void Optimiser::InterpolateDerivatives(std::vector<std::vector<int>> keyPoints, bool costDerivs){
    MatrixXd startB;
    MatrixXd endB;
    MatrixXd addB;

    double start_l_x_col1;
    double end_l_x_col1;
    double add_l_x_col1;
    double start_l_x_col2;
    double end_l_x_col2;
    double add_l_x_col2;

    MatrixXd start_l_xx_col1;
    MatrixXd end_l_xx_col1;
    MatrixXd add_l_xx_col1;
    MatrixXd start_l_xx_col2;
    MatrixXd end_l_xx_col2;
    MatrixXd add_l_xx_col2;

    // Create an array to track startIndices of next interpolation for each dof
    int startIndices[dof];
    for(int i = 0; i < dof; i++){
        startIndices[i] = 0;
    }

    // Loop through all the time indices - can skip the first
    // index as we preload the first index as the start index for all dofs.
    for(int t = 1; t < horizonLength; t++){
        // Loop through all the dofs
        for(int i = 0; i < dof; i++){
            // Check the current vector at that time segment for the current dof
            std::vector<int> columns = keyPoints[t];

            // If there are no keypoints, continue onto second run of the loop
            if(columns.size() == 0){
                continue;
            }

            for(int j = 0; j < columns.size(); j++){

                // If there is a match, interpolate between the start index and the current index
                // For the given columns
                if(i == columns[j]){
//                    cout << "dof: " << i << " end index: " << t << " start index: " << startIndices[i] << "\n";
                    MatrixXd startACol1 = A[startIndices[i]].block(dof, i, dof, 1);
                    MatrixXd endACol1 = A[t].block(dof, i, dof, 1);
                    MatrixXd addACol1 = (endACol1 - startACol1) / (t - startIndices[i]);

                    // Same again for column 2 which is dof + i
                    MatrixXd startACol2 = A[startIndices[i]].block(dof, i + dof, dof, 1);
                    MatrixXd endACol2 = A[t].block(dof, i + dof, dof, 1);
                    MatrixXd addACol2 = (endACol2 - startACol2) / (t - startIndices[i]);

                    if(costDerivs){
                        start_l_x_col1 = l_x[startIndices[i]](i, 0);
                        end_l_x_col1 = l_x[t](i, 0);
                        add_l_x_col1 = (end_l_x_col1 - start_l_x_col1) / (t - startIndices[i]);

                        start_l_x_col2 = l_x[startIndices[i]](i + dof, 0);
                        end_l_x_col2 = l_x[t](i + dof, 0);
                        add_l_x_col2 = (end_l_x_col2 - start_l_x_col2) / (t - startIndices[i]);

                        start_l_xx_col1 = l_xx[startIndices[i]].block(i, 0, 1, dof);
                        end_l_xx_col1 = l_xx[t].block(i, 0, 1, dof);
                        add_l_xx_col1 = (end_l_xx_col1 - start_l_xx_col1) / (t - startIndices[i]);

                        start_l_xx_col2 = l_xx[startIndices[i]].block(i + dof, 0, 1, dof);
                        end_l_xx_col2 = l_xx[t].block(i + dof, 0, 1, dof);
                        add_l_xx_col2 = (end_l_xx_col2 - start_l_xx_col2) / (t - startIndices[i]);
                    }

                    if(i < num_ctrl){
                        startB = B[startIndices[i]].block(dof, i, dof, 1);
                        endB = B[t].block(dof, i, dof, 1);
                        addB = (endB - startB) / (t - startIndices[i]);
                    }

                    for(int k = startIndices[i]; k < t; k++){
                        A[k].block(dof, i, dof, 1) = startACol1 + ((k - startIndices[i]) * addACol1);

                        A[k].block(dof, i + dof, dof, 1) = startACol2 + ((k - startIndices[i]) * addACol2);

                        if(costDerivs){
                            l_x[k](i) = start_l_x_col1 + ((k - startIndices[i]) * add_l_x_col1);
                            l_x[k](i + dof) = start_l_x_col2 + ((k - startIndices[i]) * add_l_x_col2);

                            l_xx[k].block(i, 0, 1, dof) = start_l_xx_col1 + ((k - startIndices[i]) * add_l_xx_col1);
                            l_xx[k].block(i + dof, 0, 1, dof) = start_l_xx_col2 + ((k - startIndices[i]) * add_l_xx_col2);
                        }

                        if(i < num_ctrl){
                            B[k].block(dof, i, dof, 1) = startB + ((k - startIndices[i]) * addB);
                        }
                    }
                    startIndices[i] = t;
                }
            }
        }
    }
}

void Optimiser::FilterDynamicsMatrices() {

    for(int i = dof; i < 2 * dof; i++){
        for(int j = 0; j < 2 * dof; j++){
            std::vector<double> unfiltered;
            std::vector<double> filtered;

            for(int k = 0; k < horizonLength; k++){
                unfiltered.push_back(A[k](i, j));
            }

            if(filteringMethod == "low_pass"){
                filtered = FilterIndValLowPass(unfiltered);
            }
            else if(filteringMethod == "FIR"){
                filtered = FilterIndValFIRFilter(unfiltered, FIRCoefficients);
            }
            else{
                std::cerr << "Filtering method not recognised" << std::endl;
            }


            for(int k = 0; k < horizonLength; k++){
                A[k](i, j) = filtered[k];
            }
        }
    }
}

std::vector<double> Optimiser::FilterIndValLowPass(std::vector<double> unfiltered){
    double yn1 = unfiltered[0];
    double xn1 = unfiltered[0];

    std::vector<double> filtered;
    for(int i = 0; i < unfiltered.size(); i++){
        double xn = unfiltered[i];

        double yn = ((1-lowPassACoefficient)*yn1) + lowPassACoefficient*((xn + xn1)/2);

        xn1 = xn;
        yn1 = yn;

        filtered.push_back(yn);
    }
    return filtered;
}

std::vector<double> Optimiser::FilterIndValFIRFilter(std::vector<double> unfiltered, std::vector<double> filterCoefficients){
    std::vector<double> filtered;

    for(int i = 0; i < unfiltered.size(); i++){
        filtered.push_back(0);
    }

    for(int i = 0; i < unfiltered.size(); i++){
        for(int j = 0; j < filterCoefficients.size(); j++){
            if(i - j >= 0){
                filtered[i] += unfiltered[i - j] * filterCoefficients[j];
            }
        }

    }
    return filtered;
}

void Optimiser::setFIRFilter(std::vector<double> _FIRCoefficients){
    FIRCoefficients.clear();

    for(int i = 0; i < _FIRCoefficients.size(); i++){
        FIRCoefficients.push_back(_FIRCoefficients[i]);
    }
}

