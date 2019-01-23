# Austenite-actuated-continuum-arm

Thesis for my MSc. Project used nitinol shape memory alloys as novel actuator for a modular snake arm robot. Thesis presents background, methods, testing and results.
Nitinol actuators are completely silent, extremely compact, and cost is comparable to existing electrical motors.

Modular chassis designed in FreeCAD. Basic testing software programmed on Arduino in C, electronic diagrams included.

Key conclusions are
- Best SMA actuators are bundles of thin (150um) wires. 
- SMAs can be modelled as a spring whose spring constant is a function of austenite to martensite ratio. Suitable for compliant actuators.
- Austenite ratio is a function of input power.
- Austenite ratio can be measured via electrical resistance (though exceptionally noisy). 3rd order elliptical filter found to reduce noise effectively.
- Heat hysteresis, the biggest drawback of SMAs, can be partially overcome with antagonistic pairs and optimal heating algorithms.
- Coil-type actuators provide largest stroke, but tend to keep heat inside the coil, exacerbating hysteresis. Improved cooling strategies could mitigate this.
