#include "stdInclude.h"
#include "ros/ros.h"
#include "fileHandler.h"

// --------------------- different scenes -----------------------
#include "doublePendulum.h"
#include "reaching.h"
#include "twoDPushing.h"
#include "twoDPushingClutter.h"

#include "visualizer.h"
#include "MuJoCoHelper.h"

#include "interpolated_iLQR.h"
#include "stomp.h"

// ------------ MODES OF OEPRATION -------------------------------
#define SHOW_INIT_CONTROLS          0
#define ILQR_ONCE                   1
#define MPC_CONTINOUS               2
#define MPC_UNTIL_COMPLETE          3
#define DEFAULT_KEYBOARD_CONTROL    4

enum scenes{
    pendulum = 0,
    reaching = 1,
    twoReaching = 2,
    cylinderPushing = 3,
    threeDPushing = 4,
    boxFlicking = 5,
    cylinderPushingClutter = 6
};

// --------------------- Global class instances --------------------------------
modelTranslator *activeModelTranslator;
differentiator *activeDifferentiator;
optimiser *activeOptimiser;
interpolatediLQR *iLQROptimiser;
stomp *stompOptimiser;
visualizer *activeVisualiser;
fileHandler yamlReader;

void showInitControls();
void iLQROnce();
void MPCUntilComplete();
void MPCContinous();
void keyboardControl();

int main(int argc, char **argv) {
    cout << "program started \n";
    std::string optimiser;
    int mode;
    int task;

    yamlReader.readSettingsFile("/generalConfig.yaml");
    optimiser = yamlReader.optimiser;
    mode = yamlReader.project_display_mode;
    task = yamlReader.taskNumber;

    scenes myScene = cylinderPushingClutter;
    MatrixXd startStateVector(1, 1);

    if(task == pendulum){
        doublePendulum *myDoublePendulum = new doublePendulum();
        activeModelTranslator = myDoublePendulum;
        startStateVector.resize(activeModelTranslator->stateVectorSize, 1);

        startStateVector = activeModelTranslator->returnRandomStartState();
        //startStateVector << 3.14, 0, 0, 0;
    }
    else if(task == reaching){
        // std::cout << "before creating reaching problem" << std::endl;
        pandaReaching *myReaching = new pandaReaching();
        activeModelTranslator = myReaching;
        startStateVector.resize(activeModelTranslator->stateVectorSize, 1);
        startStateVector = activeModelTranslator->returnRandomStartState();
    }
    else if(task == twoReaching){
        cout << "not implemented task yet " << endl;
        return -1;
    }
    else if(task == cylinderPushing){
        twoDPushing *myTwoDPushing = new twoDPushing();
        activeModelTranslator = myTwoDPushing;
        startStateVector.resize(activeModelTranslator->stateVectorSize, 1);
        startStateVector = activeModelTranslator->returnRandomStartState();

    }
    else if(task == threeDPushing){
        cout << "not implemented task yet " << endl;
        return -1;

    }
    else if(task == boxFlicking){
        cout << "not implemented task yet " << endl;
        return -1;
    }
    else if(task == cylinderPushingClutter){
        twoDPushingClutter *myTwoDPushingClutter = new twoDPushingClutter();
        activeModelTranslator = myTwoDPushingClutter;
        startStateVector.resize(activeModelTranslator->stateVectorSize, 1);
        startStateVector = activeModelTranslator->returnRandomStartState();
    }
    else{
        std::cout << "invalid scene selected, exiting" << std::endl;
    }

    activeDifferentiator = new differentiator(activeModelTranslator, activeModelTranslator->myHelper);
    activeModelTranslator->setStateVector(startStateVector, MAIN_DATA_STATE);
    cout << "starting state: " << startStateVector << endl;

    //Instantiate my optimiser
    activeVisualiser = new visualizer(activeModelTranslator);

    if(optimiser == "interpolated_iLQR"){
        yamlReader.readOptimisationSettingsFile(opt_iLQR);
        iLQROptimiser = new interpolatediLQR(activeModelTranslator, activeModelTranslator->activePhysicsSimulator, activeDifferentiator, yamlReader.maxHorizon, activeVisualiser);
        activeOptimiser = iLQROptimiser;
    }
    else if(optimiser == "stomp"){
        yamlReader.readOptimisationSettingsFile(opt_stomp);
        stompOptimiser = new stomp(activeModelTranslator, activeModelTranslator->activePhysicsSimulator, yamlReader.maxHorizon, 8);
        activeOptimiser = stompOptimiser;
    }
    else if(optimiser == "gradDescent"){
        cout << "not implemented grad descent yet " << endl;
        return -1;
    }
    else{
        cout << "invalid optimiser selected, exiting" << endl;
        return -1;
    }
    
    activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);
    activeModelTranslator->activePhysicsSimulator->appendSystemStateToEnd(MAIN_DATA_STATE);

    if(mode == SHOW_INIT_CONTROLS){
        cout << "SHOWING INIT CONTROLS MODE \n";
        showInitControls();
    }
    else if(mode == ILQR_ONCE){
        cout << "OPTIMISE TRAJECTORY ONCE AND DISPLAY MODE \n";
        iLQROnce();
    }
    else if(mode == MPC_CONTINOUS){
        cout << "CONTINOUS MPC MODE \n";
        MPCContinous();
    }
    else if(mode == MPC_UNTIL_COMPLETE){
        cout << "MPC UNTIL TASK COMPLETE MODE \n";
        MPCUntilComplete();
    }
    else if(mode == DEFAULT_KEYBOARD_CONTROL){
        cout << "KEYBOARD TESTING MODE \n";
        keyboardControl();
    }
    else{
        cout << "INVALID MODE OF OPERATION OF PROGRAM \n";
    }

    return 0;
}

void showInitControls(){
    int horizon = 2000;
    int controlCounter = 0;
    int visualCounter = 0;

    std::vector<MatrixXd> initControls = activeModelTranslator->createInitOptimisationControls(horizon);
    activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);

    while(activeVisualiser->windowOpen()){

        activeModelTranslator->setControlVector(initControls[controlCounter], MAIN_DATA_STATE);

        activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);

        controlCounter++;
        visualCounter++;

        if(controlCounter >= horizon){
            controlCounter = 0;
            activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);
        }

        if(visualCounter > 5){
            visualCounter = 0;
            activeVisualiser->render("show init controls");
        }
    }
}

void iLQROnce(){
    int horizon = 3000;
    int controlCounter = 0;
    int visualCounter = 0;
    bool showFinalControls = true;
    char* label = "Final controls";

    std::vector<MatrixXd> initControls = activeModelTranslator->createInitOptimisationControls(horizon);
    activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);
    auto start = high_resolution_clock::now();
    std::vector<MatrixXd> optimisedControls = activeOptimiser->optimise(0, initControls, yamlReader.maxIter, yamlReader.minIter, horizon);
    auto stop = high_resolution_clock::now();
    auto linDuration = duration_cast<microseconds>(stop - start);
    cout << "iLQR once took: " << linDuration.count() / 1000000.0f << " ms\n";

    activeModelTranslator->activePhysicsSimulator->copySystemState(0, MAIN_DATA_STATE);

    while(activeVisualiser->windowOpen()){

        if(showFinalControls){
            activeModelTranslator->setControlVector(optimisedControls[controlCounter], MAIN_DATA_STATE);
        }
        else{
            activeModelTranslator->setControlVector(initControls[controlCounter], MAIN_DATA_STATE);
        }
        

        activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);

        controlCounter++;
        visualCounter++;

        if(controlCounter >= horizon){
            controlCounter = 0;
            activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);
            showFinalControls = !showFinalControls;
            if(showFinalControls){
                label = "Final controls";
            }
            else{
                label = "Init Controls";
            }
        }

        if(visualCounter > 5){
            visualCounter = 0;
            activeVisualiser->render(label);
        }
    }
}

void MPCContinous(){
    int horizon = 100;
    bool taskComplete = false;
    int currentControlCounter = 0;
    int visualCounter = 0;
    int overallTaskCounter = 0;
    int reInitialiseCounter = 0;
    const char* label = "MPC Continous";

    // Instantiate init controls
    std::vector<MatrixXd> initControls;
    initControls = activeModelTranslator->createInitOptimisationControls(horizon);
    activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);
    cout << "init controls: " << initControls.size() << endl;
    std::vector<MatrixXd> optimisedControls = activeOptimiser->optimise(0, initControls, 1000, 2, horizon);

    while(!taskComplete){
        MatrixXd nextControl = optimisedControls[0].replicate(1, 1);

        optimisedControls.erase(optimisedControls.begin());

        optimisedControls.push_back(optimisedControls.at(optimisedControls.size() - 1));

        activeModelTranslator->setControlVector(nextControl, MAIN_DATA_STATE);

        activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);

        reInitialiseCounter++;
        visualCounter++;

        if(reInitialiseCounter > 1){
            //initControls = activeModelTranslator->createInitControls(horizon);
            optimisedControls = activeOptimiser->optimise(MAIN_DATA_STATE, optimisedControls, 8, 0, horizon);
            reInitialiseCounter = 0;
        }

        if(visualCounter > 5){
            activeVisualiser->render(label);
            visualCounter = 0;
        }
    }
    //finalControls = optimiser->optimise(d_init, initControls, 2, MUJ_STEPS_HORIZON_LENGTH, 5, predicted_States, K_feedback);



    // Initialise optimiser - creates all the data objects
    // cout << "X desired: " << modelTranslator->X_desired << endl;
    // optimiser->updateNumStepsPerDeriv(5);
    

    // cpMjData(model, mdata, d_init);
    // cpMjData(model, d_init_master, d_init);

    // auto MPCStart = high_resolution_clock::now();

    // while(!taskComplete){

    //     if(movingToStart){

    //     }
    //     else{

    //     }

    //     m_ctrl nextControl = finalControls.at(0);
    //     // Delete control we have applied
    //     finalControls.erase(finalControls.begin());
    //     // add control to back - replicate last control for now
    //     finalControls.push_back(finalControls.at(finalControls.size() - 1));

    //     // Store applied control in a std::vector for re-playability
    //     MPCControls.push_back(nextControl);
    //     modelTranslator->setControls(mdata, nextControl, false);
    //     modelTranslator->stepModel(mdata, 1);
    //     currentControlCounter++;
    //     overallTaskCounter++;
    //     reInitialiseCounter++;

    //     // check if problem is solved?
    //     if(modelTranslator->taskCompleted(mdata)){
    //         cout << "task completed" << endl;
    //         taskComplete = true;
    //     }

    //     // timeout of problem solution
    //     if(overallTaskCounter > 3000){
    //         cout << "task timeout" << endl;
    //         taskComplete = true;
    //     }

    //     // State we predicted we would be at at this point. TODO - always true at the moment as no noise in system
    //     m_state predictedState = modelTranslator->returnState(mdata);
    //     //Check states mismatched
    //     bool replanNeeded = false;
    //     // todo - fix this!!!!!!!!!!!!!!!!!
    //     if(modelTranslator->predictiveStateMismatch(mdata, mdata)){
    //         replanNeeded = true;
    //     }

    //     if(currentControlCounter > 300){
    //         replanNeeded = true;
    //         currentControlCounter = 0;

    //     }

    //     if(replanNeeded){
    //         cpMjData(model, d_init, mdata);
    //         if(modelTranslator->newControlInitialisationNeeded(d_init, reInitialiseCounter)){
    //             cout << "re initialise needed" << endl;
    //             reInitialiseCounter = 0;
    //             initControls = modelTranslator->initOptimisationControls(mdata, d_init);
    //             finalControls = optimiser->optimise(d_init, initControls, 2, MUJ_STEPS_HORIZON_LENGTH, 5, predicted_States, K_feedback);
    //         }
    //         else{
    //             finalControls = optimiser->optimise(d_init, finalControls, 2, MUJ_STEPS_HORIZON_LENGTH, 5, predicted_States, K_feedback);
    //         }
    //     }

    //     visualCounter++;
    //     if(visualCounter >= 20){
    //         renderOnce(mdata);
    //         visualCounter = 0;
    //     }
    // }

    // auto MPCStop = high_resolution_clock::now();
    // auto MPCDuration = duration_cast<microseconds>(MPCStop - MPCStart);
    // float trajecTime = MPCControls.size() * MUJOCO_DT;
    // cout << "duration of MPC was: " << MPCDuration.count()/1000 << " ms. Trajec length of " << trajecTime << " s" << endl;

    // renderMPCAfter();
}

void MPCUntilComplete(){
    int horizon = 500;
    bool taskComplete = false;
    int currentControlCounter = 0;
    int visualCounter = 0;
    int overallTaskCounter = 0;
    int reInitialiseCounter = 0;
    const char* label = "MPC Continous";

    // Instantiate init controls
    std::vector<MatrixXd> initControls;
    initControls = activeModelTranslator->createInitOptimisationControls(horizon);
    activeModelTranslator->activePhysicsSimulator->copySystemState(MAIN_DATA_STATE, 0);
    cout << "init controls: " << initControls.size() << endl;
    std::vector<MatrixXd> optimisedControls = activeOptimiser->optimise(0, initControls, 1000, 2, horizon);

    while(!taskComplete){
        MatrixXd nextControl = optimisedControls[0].replicate(1, 1);

        optimisedControls.erase(optimisedControls.begin());

        optimisedControls.push_back(optimisedControls.at(optimisedControls.size() - 1));

        activeModelTranslator->setControlVector(nextControl, MAIN_DATA_STATE);

        activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);

        reInitialiseCounter++;
        visualCounter++;

        if(reInitialiseCounter > 50){
            initControls = activeModelTranslator->createInitOptimisationControls(horizon);
            optimisedControls = activeOptimiser->optimise(MAIN_DATA_STATE, initControls, 8, 0, horizon);
            reInitialiseCounter = 0;
        }

        if(visualCounter > 5){
            activeVisualiser->render(label);
            visualCounter = 0;
        }
    }

}

void keyboardControl(){
    
    while(activeVisualiser->windowOpen()){
        vector<double> gravCompensation;
        activeModelTranslator->activePhysicsSimulator->getRobotJointsGravityCompensaionControls("panda", gravCompensation, MAIN_DATA_STATE);
        MatrixXd control(activeModelTranslator->num_ctrl, 1);
        for(int i = 0; i < activeModelTranslator->num_ctrl; i++){
            control(i) = gravCompensation[i];
        }
        cout << "control: " << control << endl;
        activeModelTranslator->setControlVector(control, MAIN_DATA_STATE);
        activeModelTranslator->activePhysicsSimulator->stepSimulator(1, MAIN_DATA_STATE);

        activeVisualiser->render("keyboard control");
    }
}