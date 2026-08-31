BackBuddy

A wearable posture-monitoring device built on an ESP32 microcontroller. BackBuddy tracks the wearer's back angle in real time and, after sustained slouching, alerts them to correct their posture, helping build better postural habits through daily wear.

Overview

Slouching is easy to fall into and hard to notice in the moment. BackBuddy addresses this by continuously tracking the wearer's spine angle against their own calibrated upright baseline and alerting them only when poor posture is sustained, avoiding constant false alarms while still catching genuine slouching.

How It Works
Personalized calibration. On startup, the wearer sits upright and the device samples their natural posture to set an individual baseline tilt, so BackBuddy adapts to each user rather than assuming a fixed "correct" angle.
Real-time monitoring. An onboard IMU measures tilt continuously. The firmware computes the angle between the sensor's axes using atan2 and compares it against the baseline.
Debounced alerting. A slouch only counts once posture deviates more than 15 degrees from baseline and stays there for 10 continuous seconds, filtering out brief, natural movements. Sustained slouching triggers a haptic vibration cue prompting the wearer to straighten up.
Hardware
ESP32 microcontroller
MPU-9250 9-axis IMU (accelerometer / gyroscope / magnetometer), connected over I2C (SDA pin 21, SCL pin 22)
Vibration motor for haptic feedback
Software

Written in C++ on the Arduino framework (backbuddy.ino). Key features:

I2C sensor interfacing via the MPU9250_asukiaaa library
Accelerometer-based tilt calculation
User-specific baseline calibration routine
Time-thresholded slouch detection to minimize false positives
Status

Active, in-progress project. The sensing, calibration, and slouch-detection logic are implemented; current work focuses on driving the physical vibration motor from the detection trigger and moving from a wired, serial-monitored prototype to a self-contained wearable. Planned improvements include adjustable sensitivity, battery power, and optional session data logging.

Authors

Nirupam Chalamala and Niharika Chalamala
