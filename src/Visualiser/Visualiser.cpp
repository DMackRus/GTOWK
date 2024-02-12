//
// Created by dave on 07/03/23.
//

#include "Visualiser.h"

Visualiser::Visualiser(std::shared_ptr<ModelTranslator> _modelTranslator){

    MuJoCo_helper = _modelTranslator->MuJoCo_helper;
    activeModelTranslator = _modelTranslator;

    if (!glfwInit())
        mju_error("Could not initialize GLFW");
    window = glfwCreateWindow(1200, 900, "MuJoCo", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    MuJoCo_helper->initVisualisation();

    // Set window pointer to this class
    glfwSetWindowUserPointer(window, this);
    // install GLFW mouse and keyboard callbacks
    glfwSetKeyCallback(window, keyboardCallbackWrapper);
    glfwSetCursorPosCallback(window, mouseMoveCallbackWrapper);
    glfwSetMouseButtonCallback(window, mouseButtonCallbackWrapper);
    glfwSetScrollCallback(window, scrollCallbackWrapper);
    glfwSetWindowCloseCallback(window, windowCloseCallbackWrapper);

}

// ------------------------------- Keyboard Callback -------------------------------------------------
void Visualiser::keyboardCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods){
    Visualiser* myVisualizer = static_cast<Visualiser*>(glfwGetWindowUserPointer(window));
    myVisualizer->keyboard(window, key, scancode, action, mods);
}

void Visualiser::keyboard(GLFWwindow* window, int key, int scancode, int act, int mods){
    // backspace: reset simulation


    if (act == GLFW_PRESS && key == GLFW_KEY_BACKSPACE)
    {
        replayTriggered = true;
    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_P){

        MuJoCo_helper->appendSystemStateToEnd(MAIN_DATA_STATE);
    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_O){

        MuJoCo_helper->copySystemState(MAIN_DATA_STATE, 0);
    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_Q){

        std::cout << "before ut " << std::endl;
        MatrixXd Xt(activeModelTranslator->state_vector_size, 1);
        MatrixXd X_last(activeModelTranslator->state_vector_size, 1);
        MatrixXd Ut(activeModelTranslator->num_ctrl, 1);
        MatrixXd U_last(activeModelTranslator->num_ctrl, 1);

        Xt = activeModelTranslator->ReturnStateVector(MAIN_DATA_STATE);
        X_last = Xt.replicate(1, 1);
        double cost = activeModelTranslator->CostFunction(MAIN_DATA_STATE, false);
        std::cout << "cost: " << cost << std::endl;


        MatrixXd l_x, l_xx, l_u, l_uu;
        activeModelTranslator->CostDerivatives(MAIN_DATA_STATE, l_x, l_xx, l_u, l_uu, false);
        cout << "l_x: " << l_x << endl;
        cout << "l_xx:" << l_xx << endl;

        MatrixXd posVector, velVector, accelVec, stateVector;
        posVector = activeModelTranslator->returnPositionVector(MAIN_DATA_STATE);
        velVector = activeModelTranslator->returnVelocityVector(MAIN_DATA_STATE);
        stateVector = activeModelTranslator->ReturnStateVector(MAIN_DATA_STATE);
        accelVec = activeModelTranslator->returnAccelerationVector(MAIN_DATA_STATE);
        cout << "pos Vector: " << posVector << endl;
        cout << "vel vector: " << velVector << endl;
        cout << "stateVector: " << stateVector << endl;
        cout << "accel vector: " << accelVec << endl;
        
    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_W){
        MatrixXd controlVec;
//        SensorByName(model, data, "torso_position")[2];
        MuJoCo_helper->sensorState(MAIN_DATA_STATE, "torso_position");
        controlVec = activeModelTranslator->ReturnControlVector(0);
        cout << "control vec: " << controlVec << endl;

    }
        // left arrow key pressed
    else if(act == GLFW_PRESS && key == GLFW_KEY_A){
        // Analyse a specific stored system state


        int dataIndex = 1;

        MatrixXd Xt(activeModelTranslator->state_vector_size, 1);
        MatrixXd X_last(activeModelTranslator->state_vector_size, 1);
        MatrixXd Ut(activeModelTranslator->num_ctrl, 1);
        MatrixXd U_last(activeModelTranslator->num_ctrl, 1);


        Ut = activeModelTranslator->ReturnControlVector(dataIndex);
        U_last = activeModelTranslator->ReturnControlVector(dataIndex);

        Xt = activeModelTranslator->ReturnStateVector(dataIndex);
        X_last = Xt.replicate(1, 1);
        double cost = activeModelTranslator->CostFunction(dataIndex, false);
        cout << "------------------------------------------------- \n";
        std::cout << "cost: " << cost << std::endl;


        MatrixXd l_x, l_xx, l_u, l_uu;
        activeModelTranslator->CostDerivatives(dataIndex, l_x, l_xx, l_u, l_uu, false);
        cout << "l_x: " << l_x << endl;
        cout << "l_xx:" << l_xx << endl;

        MatrixXd posVector, velVector, accelVec, stateVector;
        posVector = activeModelTranslator->returnPositionVector(dataIndex);
        velVector = activeModelTranslator->returnVelocityVector(dataIndex);
        stateVector = activeModelTranslator->ReturnStateVector(dataIndex);
        accelVec = activeModelTranslator->returnAccelerationVector(dataIndex);
        cout << "pos Vector: " << posVector << endl;
        cout << "vel vector: " << velVector << endl;
        cout << "stateVector: " << stateVector << endl;
        cout << "accel vector: " << accelVec << endl;
        cout << "------------------------------------------------- \n";


    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_S){
        // cout << "finite differencing test \n";
        // MatrixXd A, B;
        // int dataIndex = 0;
        // activeDifferentiator->getDerivatives(A, B, false, dataIndex);

        // cout << "----------------B ------------------ \n";
        // cout << B << endl;
        // cout << "--------------- A ---------------------- \n";
        // cout << A << endl;

    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_Z){
        // Print screen view settings


    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_X){

    }

//     if up arrow key pressed
    else if(act == GLFW_PRESS && key == GLFW_KEY_UP){
        for(int i = 0; i < 10; i++){
            MatrixXd vel = activeModelTranslator->returnVelocityVector(MAIN_DATA_STATE);
            vel(0) = testVel;
            cout << "vel: " << vel << endl;
            activeModelTranslator->setVelocityVector(vel, MAIN_DATA_STATE);
            MuJoCo_helper->stepSimulator(1, MAIN_DATA_STATE);
        }
        
    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_DOWN){

    }
        // left arrow key pressed
    else if(act == GLFW_PRESS && key == GLFW_KEY_LEFT){
        MatrixXd vel = activeModelTranslator->returnVelocityVector(MAIN_DATA_STATE);
        vel(0) -= 0.1;
        testVel = vel(0);
        cout << testVel << endl;
        activeModelTranslator->setVelocityVector(vel, MAIN_DATA_STATE);

    }
    else if(act == GLFW_PRESS && key == GLFW_KEY_RIGHT){
        MatrixXd vel = activeModelTranslator->returnVelocityVector(MAIN_DATA_STATE);
        vel(0) += 0.1;
        testVel = vel(0);
        cout << "vel(0) " << vel(0) << endl;
        cout << "testVel " << testVel << endl;
        activeModelTranslator->setVelocityVector(vel, MAIN_DATA_STATE);

    }

}
// -----------------------------------------------------------------------------------------------------

// ------------------------------- Mouse button Callback -----------------------------------------------
void Visualiser::mouseButtonCallbackWrapper(GLFWwindow* window, int button, int action, int mods){
    Visualiser* myVisualizer = static_cast<Visualiser*>(glfwGetWindowUserPointer(window));
    myVisualizer->mouse_button(window, button, action, mods);
}

// mouse button callback
void Visualiser::mouse_button(GLFWwindow* window, int button, int act, int mods){
    // update button state
    button_left = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    button_middle = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    button_right = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    // update mouse position
    glfwGetCursorPos(window, &lastx, &lasty);
}
// -----------------------------------------------------------------------------------------------------


// ------------------------------- Mouse move Callback ------------------------------------------------

void Visualiser::mouseMoveCallbackWrapper(GLFWwindow* window, double xpos, double ypos){
    Visualiser* myVisualizer = static_cast<Visualiser*>(glfwGetWindowUserPointer(window));
    myVisualizer->mouse_move(window, xpos, ypos);
}

void Visualiser::mouse_move(GLFWwindow* window, double xpos, double ypos){
    // no buttons down: nothing to do
    if (!button_left && !button_middle && !button_right)
        return;

    // compute mouse displacement, save
    double dx = xpos - lastx;
    double dy = ypos - lasty;
    lastx = xpos;
    lasty = ypos;

    MuJoCo_helper->mouseMove(dx, dy, button_left, button_right, window);
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------- Scroll Callback ---------------------------------------------------
void Visualiser::scrollCallbackWrapper(GLFWwindow* window, double xoffset, double yoffset){
    Visualiser* myVisualizer = static_cast<Visualiser*>(glfwGetWindowUserPointer(window));
    myVisualizer->scroll(window, xoffset, yoffset);
}
// scroll callback
void Visualiser::scroll(GLFWwindow* window, double xoffset, double yoffset){
    // emulate vertical mouse motion = 5% of window height
    MuJoCo_helper->scroll(yoffset);
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------- Window close callback -----------------------------------------------
void Visualiser::windowCloseCallbackWrapper(GLFWwindow* window){
    Visualiser* myVisualizer = static_cast<Visualiser*>(glfwGetWindowUserPointer(window));
    myVisualizer->windowCloseCallback(window);
}

void Visualiser::windowCloseCallback(GLFWwindow * /*window*/) {
    // Use this flag if you wish not to terminate now.
    // glfwSetWindowShouldClose(window, GLFW_FALSE);


//    mjv_freeScene(&scn);
//    mjr_freeContext(&con);
//
//    // free MuJoCo model and data, deactivate
//    mj_deleteData(mdata);
//    mj_deleteModel(model);
//    mj_deactivate();
}
// ----------------------------------------------------------------------------------------------------

bool Visualiser::windowOpen(){
    return !glfwWindowShouldClose(window);
}

void Visualiser::render(const char* label) {

    MuJoCo_helper->updateScene(window, label);
    glfwSwapBuffers(window);
    glfwPollEvents();
}