# iqrf_guard
IQRF guard devices

Realizes communication between two devices - alarm detector and alarm executor


Slave detects an anomaly (PIR sensor, other sensors) and sends a message to the master

Master receives the messsage coming from slave and decides how to control alarm device (camera taking picture, a loud beeper, other alarm-giving device). Master can acknowledge the message to the slave or not (source files with no "noack" string)
