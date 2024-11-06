#include "ModelTranslator/PlaceObject.h"

PlaceObject::PlaceObject(std::string EE_name, std::string body_name){
    this->EE_name = EE_name;
    this->body_name = body_name;

    std::string yamlFilePath = "/TaskConfigs/rigid_body_manipulation/place_single.yaml";
    InitModelTranslator(yamlFilePath);
}

std::vector<MatrixXd> PlaceObject::CreateInitOptimisationControls(int horizonLength) {
    std::vector<MatrixXd> init_opt_controls;
    int num_ctrl = current_state_vector.num_ctrl;
    vector<double> gravCompensation;
    MatrixXd control(num_ctrl, 1);

    for(int i = 0; i < horizonLength; i++){

        MuJoCo_helper->GetRobotJointsGravityCompensationControls(current_state_vector.robots[0].name, gravCompensation, MuJoCo_helper->main_data);

        for(int j = 0; j < num_ctrl; j++){
            control(j) = gravCompensation[j];
        }

        SetControlVector(control, MuJoCo_helper->main_data, full_state_vector);
        mj_step(MuJoCo_helper->model, MuJoCo_helper->main_data);
        init_opt_controls.push_back(control);
    }

    return init_opt_controls;
}

//std::vector<MatrixXd> PlaceObject::CreateInitSetupControls(int horizonLength) {
//
//}

void PlaceObject::Residuals(mjData *d, MatrixXd &residuals) {
    int resid_index = 0;

    // Compute kinematics chain to compute site poses
//    mj_kinematics(MuJoCo_helper->model, d);
    mj_forwardSkip(MuJoCo_helper->model, d, mjSTAGE_NONE, 1);

    pose_6 goal_pose;
    pose_6 goal_velocity;
    pose_6 ee_pose;
    MuJoCo_helper->GetBodyPoseAngle("goal", goal_pose, d);
    MuJoCo_helper->GetBodyVelocity("goal", goal_velocity, d);

    int site_id = mj_name2id(MuJoCo_helper->model, mjOBJ_SITE, "end_effector");
    for(int i = 0; i < 3; i++){
        ee_pose.position(i) = d->site_xpos[site_id * 3 + i];
    }

    // --------------- Residual 0: Body goal position -----------------
    double diff_x = goal_pose.position(0) - residual_list[0].target[0];
    double diff_y = goal_pose.position(1) - residual_list[0].target[1];
    double diff_z = goal_pose.position(2) - residual_list[0].target[2];
    residuals(resid_index++, 0) = sqrt(pow(diff_x, 2)
                                       + pow(diff_y, 2)
                                       + pow(diff_z, 2));

    // --------------- Residual 1: Body goal velocity -----------------
    diff_x = goal_velocity.position(0);
    diff_y = goal_velocity.position(1);
    diff_z = goal_velocity.position(2);
    residuals(resid_index++, 0) = sqrt(pow(diff_x, 2)
                                       + pow(diff_y, 2)
                                       + pow(diff_z, 2));

    // --------------- Residual 2: EE - body -----------------
    diff_x = goal_pose.position(0) - ee_pose.position(0);
    diff_y = goal_pose.position(1) - ee_pose.position(1);
    diff_z = goal_pose.position(2) - ee_pose.position(2);
    residuals(resid_index++, 0) = pow(sqrt(pow(diff_x, 2)
                                       + pow(diff_y, 2)
                                       + pow(diff_z, 2)) - residual_list[2].target[0], 2);

    if(resid_index != residual_list.size()){
        std::cerr << "Error: Residuals size mismatch\n";
        exit(1);
    }
}

bool PlaceObject::TaskComplete(mjData *d, double &dist) {
    return false;
}

void PlaceObject::SetGoalVisuals(mjData *d) {
    pose_6 goal_pose;
    MuJoCo_helper->GetBodyPoseAngle("target", goal_pose, d);

    // Set the goal object position
    goal_pose.position(0) = residual_list[0].target[0];
    goal_pose.position(1) = residual_list[0].target[1];
    goal_pose.position(2) = 0.032;
    MuJoCo_helper->SetBodyPoseAngle("target", goal_pose, d);

    // TODO - not sure about this
    // Activate pump adhesion - Pump adhesion is not a decision variable currently in optimisation
    int pump_id = mj_name2id(MuJoCo_helper->model, mjOBJ_ACTUATOR, "adhere_pump");
    d->ctrl[pump_id] = 4.0;
}