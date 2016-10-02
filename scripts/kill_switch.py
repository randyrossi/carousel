import RPi.GPIO as GPIO
import time
import datetime
import os

GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.IN, pull_up_down = GPIO.PUD_UP)

input = 1

time.sleep(1)

while True:

  while input == 1:
    input = GPIO.input(18)
    time.sleep(.1)

  os.system("sudo killall advmame > /dev/null")
  os.system("sudo killall daphne > /dev/null")
  os.system("sudo killall mame > /dev/null")

  rebooting = 0
  input = 0
  held_begin = time.time()
  while input == 0:
    input = GPIO.input(18)
    time.sleep(.1)
    held_now = time.time()
    if (rebooting == 0 and held_now - held_begin >= 3):
      os.system("sudo shutdown -h now > /dev/null");
      rebooting = 1
