# ithopwm
Drive an ITHO CVE ECO RFT with PWM print using ESP8266

Please note that the ITHO board specs ask for 5V-10V, 3.5kHz-4.5kHz; however 5V is not enough so drive it at 7.5v or 9v. Not all ESP boards can do that! Amica NodeMCU v2 does so that's the preferred choice.

We're using 4kHz PWM in this example. We dim the led based on the duty cycle as well.

Lastly, the code was written for a NodeMCU Amica v2 board, may need adaptation for others.
