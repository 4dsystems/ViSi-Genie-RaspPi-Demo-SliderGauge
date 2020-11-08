/*
 * demo2.c:
 *	Demonstration program to listen to a 4D Systems panel
 *	with the 2nd button + single slider demo installed and control
 *	some LEDs connected to the GPIO port on the Raspberry Pi
 *
 *	Gordon Henderson, December 2012
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <wiringPi.h>
#include <softPwm.h>

#include <geniePi.h>


/*
 * logLed:
 *	Simple attempt to make the scale logarithmic - stops the LEDs
 *	becoming too bright too soon.
 *********************************************************************************
 */

static int logLed (int value)
{
  int result ;

  /**/ if (value == 0)
    result =   0 ;
  else if (value == 100)
    result = 100 ;
  else
    result = (int)rint (100.0 - log10 ((double)(100 - value)) * 50.0) ;

  return result ;
}


/*
 * setLEDs:
 *	Set our PWM LEDs and reflect the slider values too
 *********************************************************************************
 */

static void setLEDs (int r, int g, int b)
{
  softPwmWrite (0, logLed (r)) ;
  softPwmWrite (1, logLed (g)) ;
  softPwmWrite (2, logLed (b)) ;
}


/*
 * resetDisplay:
 *********************************************************************************
 */

static void resetDisplay (void)
{
  setLEDs (0, 0, 0) ;

  genieWriteObj (GENIE_OBJ_SLIDER, 0, 0) ;
  genieWriteObj (GENIE_OBJ_SLIDER, 1, 0) ;
  genieWriteObj (GENIE_OBJ_SLIDER, 2, 0) ;

  genieWriteObj (GENIE_OBJ_GAUGE,         0, 0) ;
  genieWriteObj (GENIE_OBJ_ANGULAR_METER, 0, 0) ;
  genieWriteObj (GENIE_OBJ_COOL_GAUGE,    0, 0) ;
}


/*
 * handleGenieEvent:
 *	Take a reply off the display and call the appropriate handler for it.
 *********************************************************************************
 */

void handleGenieEvent (struct genieReplyStruct *reply)
{
  int r, g, b ;

  if (reply->cmd != GENIE_REPORT_EVENT)
  {
    printf ("Invalid event from the display: 0x%02X\r\n", reply->cmd) ;
    return ;
  }

  /**/ if (reply->object == GENIE_OBJ_WINBUTTON)
  {
    /**/ if (reply->index == 0)
      resetDisplay () ;
    else if (reply->index == 1)
    {
      r = random () % 101 ;
      g = random () % 101 ;
      b = random () % 101 ;
      setLEDs (r, g, b) ;

      genieWriteObj (GENIE_OBJ_SLIDER, 0, r) ;
      genieWriteObj (GENIE_OBJ_SLIDER, 1, g) ;
      genieWriteObj (GENIE_OBJ_SLIDER, 2, b) ;

      genieWriteObj (GENIE_OBJ_GAUGE,         0, r) ;
      genieWriteObj (GENIE_OBJ_ANGULAR_METER, 0, g) ;
      genieWriteObj (GENIE_OBJ_COOL_GAUGE,    0, b) ;
    }
    else if (reply->index == 2)
    {
      setLEDs (100, 100, 100) ;

      genieWriteObj (GENIE_OBJ_SLIDER, 0, 100) ;
      genieWriteObj (GENIE_OBJ_SLIDER, 1, 100) ;
      genieWriteObj (GENIE_OBJ_SLIDER, 2, 100) ;

      genieWriteObj (GENIE_OBJ_GAUGE,         0, 100) ;
      genieWriteObj (GENIE_OBJ_ANGULAR_METER, 0, 100) ;
      genieWriteObj (GENIE_OBJ_COOL_GAUGE,    0, 100) ;
    }
    else
      printf ("*** Unhandled WINBUTTON event: %d, %d\n", reply->index, reply->data) ;

  }
  else if (reply->object == GENIE_OBJ_SLIDER)
  {
    /**/ if (reply->index == 0)
      softPwmWrite (0, logLed (reply->data)) ;
    else if (reply->index == 1)
      softPwmWrite (1, logLed (reply->data)) ;
    else if (reply->index == 2)
      softPwmWrite (2, logLed (reply->data)) ;
    else
      printf ("*** Unhandled SLIDER event: %d, %d\n", reply->index, reply->data) ;
  }
  else
    printf ("Unhandled Event: object: %2d, index: %d data: %d [%02X %02X %04X]\r\n",
      reply->object, reply->index, reply->data, reply->object, reply->index, reply->data) ;
}


/*
 *********************************************************************************
 * main:
 *	Run our little demo
 *********************************************************************************
 */

int main ()
{
  struct genieReplyStruct reply ;

  printf ("\n\n\n\n") ;
  printf ("Visi-Genie Slider Demo number 2\n") ;
  printf ("=============================\n") ;


// Standard wiringPi Setup

  if (wiringPiSetup () == -1)
  {
    fprintf (stderr, "Can't initialise wiringPi: %s\n", strerror (errno)) ;
    return 1 ;
  }

// Create our 3 PWM channels via the wiringPi softPwm module
//	Note that the sliders on the display were created with a range
//	of 0-100 - this maps to the softPWM range of 0-100 exactly.

  softPwmCreate (0, 0, 100) ;	// Red
  softPwmCreate (1, 0, 100) ;	// Green
  softPwmCreate (2, 0, 100) ;	// Blue

// Genie display setup
//	Using the Raspberry Pi's on-board serial port.

  if (genieSetup ("/dev/ttyS0", 115200) < 0)
  {
    fprintf (stderr, "rgb: Can't initialise Genie Display: %s\n", strerror (errno)) ;
    return 1 ;
  }

// Make sure we're displaying form 0 (the first one!)
//	and set the slider, LEDs, Gauges, etc. to zero.

  genieWriteObj (GENIE_OBJ_FORM, 0, 0) ;
  resetDisplay () ;

// Now just poll the display for an event

  for (;;)
  {
    while (genieReplyAvail ())
    {
      genieGetReply    (&reply) ;
      handleGenieEvent (&reply) ;
    }

    delay (10) ; // Don't hog the CPU in-case anything else is happening...
  }

  return 0 ;
}
