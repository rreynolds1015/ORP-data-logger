ORP-data-logger
===============

Arduino based ORP data logger

Project uses Arduino UNO, Atlas Scientific ORP module, DS1307 RTC, openLog data logger, and LinkSprite LCD button shield.

System should continuously update LCD to show current date, time, and ORP data.  Write to serial should be comma delimited
and have >= 5 seconds between data points

Button shield will allow calibration of ORP sensors and setting data logging frequency and turn logging on/off.
