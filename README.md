# Motorized_Bike_Ignition_System
Created to replace the stock ignition with minimal modification and increase performance.

# Checkpoints
-Functional Code written and working: Complete
-Prototype schematic created: Work in Progress
-Bench tested in lab: Complete
-Simulated on computer: Work in Progess

# Current Progress
We have done extensive bench testing and accuracy is within 20 microseconds (20/1,000,000 seconds). 
The next stage is building an interface between the output and coil. We are using a generic COP.

# Requirements
The system runs natively on teensy, we have shifted controllers due to limitations of the code
without extra libraries and the speed of the cheap arduinos. Teensy's are accessable high power controllers.

They run alongside a 3D printed hall sensor mount holding a sensor for timing.
You will need at least some circuitry knowledge, as well as a computer and soldering supplies.
