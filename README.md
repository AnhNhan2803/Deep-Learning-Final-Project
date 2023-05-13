# README for fall detection - Final Project - Deep Learning class
1. Environment setup and program execution
- Since I developed the fall detection project with a contribution of Tensorflow Lite, I have to copy my project to the tflite-micro repo (https://github.com/tensorflow/tflite-micro) to utilize the BAZEL build system (code build+running+testing), I placed the fall detection project in the relative path of "tensorflow/lite/micro/examples". THIS IS THE PRIMARY CONDITION TO RUN THIS PROJECT

- In summary, there are 5 main scripts that we need to care about:
+ A python script for model tuning "tuning.py" 
+ A python script for model training "train.py"
+ A python script to extract the fall events from SmartFall datasets "extractFallEvent.py" 
+ A python script for fall detection testing on PC "main.py" (This need the streaming data from the microcontroller, just to evaluate how the model perform before deploy to the Raspberry Pico W)
+ A python script for real time IMU data visualization on PC "analyzeIMUData.py" (Again, this also needs the streaming data from the microcontroller)

- For data analysis, the SmartFall datasets used for training and testing were stored in "data/training/". For the visualization of prepocessed data and the evaluation for initial and optimal model, all related figures were placed in "figures/".


- To run the program, firstly we need to install Bazel, tensorflow, tflite-runtime, and some other dependencies like scipy, scikit-learn, pandas. Just let me know via nhan.cao@umconnect.umt.edu if you can not successfully setup the environment. For the above 5 python scripts, we can run them with using bazel with the syntax "bazel run :script_name", E.g. execute "$ bazel run :train" to start the model training. Regarding the microcontroller, after training the model with "$ bazel run :train", we need to copy the model array (or TinyML array) in the file "model/fall_detection_model.h" to the C header file in "pico_w/model/tflite.h", after that move to the folder "pico_w/build" and type "$ cmake .." + "$ make -j4" to compile the firmware for the Raspberry Pico W. Then flash the firmware "*.uf2" to the Pico W and that's all.

2. Technical note
# Convert tensorflow lite model to the array of c/c++
xxd -i fall_detection_model.tflite > fall_detection_model.h

# Downgrade protobuf to run the python script with bazel "E.g. bazel build :script"
pip install "protobuf==3.20.*"

# Update protobuf to run the python script with "E.g. python script.py"
pip install --upgrade protobuf

# SmartFall datasets link
https://userweb.cs.txstate.edu/~hn12/data/SmartFallDataSet/

# Bind physical MAC address of HC-05 to rfcomm channel on linux
sudo rfcomm bind /dev/rfcomm0 98:D3:41:FD:44:A5
