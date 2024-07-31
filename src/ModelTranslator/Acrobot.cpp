#include "ModelTranslator/Acrobot.h"

Acrobot::Acrobot(): ModelTranslator(){
    std::string yamlFilePath = "/TaskConfigs/toys/acrobot.yaml";
    InitModelTranslator(yamlFilePath);
}

bool Acrobot::TaskComplete(mjData *d, double &dist){

    dist = 0.0;
    std::vector<double> acrobot_joints;
    MuJoCo_helper->GetRobotJointsPositions("acrobot", acrobot_joints, d);

    for(int i = 0; i < full_state_vector.dof; i++){
        std::cout << "joint pos " << acrobot_joints[i] << "\n";
        dist += abs(current_state_vector.robots[0].goal_pos[i] - acrobot_joints[i]);
    }

    if(dist < 0.01){
        return true;
    }

    return false;
}

void Acrobot::Residuals(mjData *d, MatrixXd &residuals){
//    MatrixXd residuals(residual_list.size(), 1);
    int resid_index = 0;

    std::vector<double> acrobot_joints;
    std::vector<double> acrobot_velocities;
    std::vector<double> acrobot_control;
    MuJoCo_helper->GetRobotJointsPositions("acrobot", acrobot_joints, d);
    MuJoCo_helper->GetRobotJointsVelocities("acrobot", acrobot_velocities, d);
    MuJoCo_helper->GetRobotJointsControls("acrobot", acrobot_control, d);

    // --------------- Residual 0: Joint 0 position -----------------
    residuals(resid_index++, 0) = acrobot_joints[0] - residual_list[0].target[0];

    // --------------- Residual 1: Joint 1 position -----------------
    residuals(resid_index++, 0) = acrobot_joints[1] - residual_list[1].target[0];

    // --------------- Residual 2: Joint 0 velocity -----------------
    residuals(resid_index++, 0) = acrobot_velocities[0] - residual_list[2].target[0];

    // --------------- Residual 3: Joint 1 velocity -----------------
    residuals(resid_index++, 0) = acrobot_velocities[1] - - residual_list[3].target[0];

    // --------------- Residual 4: Joint 0 control -----------------
    residuals(resid_index++, 0) = acrobot_control[0] - residual_list[4].target[0];

    if(resid_index != residual_list.size()){
        std::cerr << "Error: Residuals size mismatch\n";
        exit(1);
    }

//    return residuals;
}

//MatrixXd Acrobot::ReturnRandomStartState(){
//    MatrixXd randomStartState(state_vector_size, 1);
//
//    float arm1Pos = randFloat(0, 3);
//    float arm2Pos = randFloat(0, 3);
//
//    randomStartState << arm1Pos, arm2Pos, 0, 0;
//
//    return randomStartState;
//}
//
//MatrixXd Acrobot::ReturnRandomGoalState(MatrixXd X0){
//    MatrixXd randomGoalState(state_vector_size, 1);
//
//    float randomNum = randFloat(0, 1);
//    // stable down position
//    if(randomNum < 0.33){
//        randomGoalState << 3.1415, 0, 0, 0;
//    }
//        // Half up unstable
//    else if(randomNum > 0.33 && randomNum < 0.66){
//        randomGoalState << 3.1415, 3.1415, 0, 0;
//    }
//        // Unstable up position
//    else{
//        randomGoalState << 0, 0, 0, 0;
//    }
//
//    return randomGoalState;
//}
