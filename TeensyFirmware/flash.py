import RPi.GPIO as GPIO
 
# to use Raspberry Pi board pin numbers
GPIO.setmode(GPIO.BOARD)
 
# set up the GPIO channels - one input and one output
GPIO.setup(7, GPIO.OUT)
GPIO.output(7, GPIO.LOW)
GPIO.setup(7, GPIO.IN)
