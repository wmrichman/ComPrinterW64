# ComPrinterW64
A quick port of Ted Burke's very useful ComPrinter to Visual Studio C++. 

ComPrinter is a console application (i.e. a command line program) that opens a serial port and displays incoming text characters in the console. It features several very useful options, including the following:

ComPrinter supports a user-specified COM port number.
ComPrinter supports a user-specified baud rate.
By default, ComPrinter opens the highest available COM port (especially useful if you’re using a USB-to-serial converter).
ComPrinter provides a facility to simulate a keystroke for each incoming character. In combination with a USB-to-serial converter, this provides a very simple way of turning a microcontroller system with serial output into a communication device.
It provides an ideal free alternative to HyperTerminal in many applications.

Support ComPrinter Development
ComPrinter is completely free to download and use. 

Ted requests, "You can support ComPrinter development by letting me know if you find it useful." You can do so on his web page at https://batchloaf.wordpress.com/comprinter/

This version includes the additions he describes in the discussion on his page. 
<br>
<b> Again, this was all Ted's work.</b>

I’ve added four new options for causing ComPrinter to exit.
<ul>
<li>1. Exit after a certain number of characters.</li>
  For example, to exit after 5 characters: ComPrinter /charcount 5 
<br>
<li>2. Exit after a timeout – i.e. no data received for the specified number of milliseconds.</li>
  For example, to exit after 2 seconds of no data: ComPrinter /timeout 2000 
<br>
<li>3. Exit when a certain character is received.</li>
  For example, to exit when the letter ‘x’ is received:  ComPrinter /endchar x 
<br>
<li>4. Exit when a certain hex byte is received.</li>
  For example, to exit when the hex value 0xFF is received: ComPrinter /endhex FF
 </ul>
-----------------------------------------------------------------------------------------------------------------

Thanks again for the awesome utility, Ted!  I use it in a batch file to automate programming a processor via the automation 
commands available with the STM utility STM32_Programmer_CLI.exe included with the STM32CubeProgrammer.  The batch file is run
on the PC, and using ComPrinter, it waits until the attached Arduino that runs the rest of the test fixture sends it a Ctrl-z, then 
runs the programming utility and loops around waiting for another one:

:top<br>
REM Wait for Ctrl-Z from Arduino on Com4 before proceeding (and don't stuff received characters into the keyboard buffer!)<br>
comprinterw64-Arduino /devnum 4 /baudrate 19200 /endhex 1A /quiet /keypress<br>
"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=swd -e all -d <files and addresses><br>
<br>
if ERRORLEVEL 0 goto top<br>
echo "Something went wrong!!!"<br>
pause<br>
goto top<br>
    
     
