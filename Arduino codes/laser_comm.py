# -*- coding: utf-8 -*-
"""
Created on Mon May 27 10:32:08 2019

@author: Aki Ruhtinas

Python code to interact with EKSPLA NL301 Laser

"""
import time
import serial


# configure the serial connections
ser = serial.Serial(port='COM1',
    baudrate=19200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

ser.isOpen()

print('Enter your commands below.\r\n')
print('Commands are:\r\nSAY: Check laser status\r\nSTART: Start the laser\r\nSTOP: Stop the laser\r\n')
print('MAX: Set output mode to MAX\r\nADJ: Set output mode to adj\r\nOFF: Set electroptics off\r\n')
print('List of commands available with HELP command')
print('Exit application with EXIT command')

inputdata=1
try:
    while 1 :
        # get keyboard input
        inputmsg = input("PC >> ")
        if inputmsg == 'EXIT':
            ser.close()
            exit()
        elif inputmsg =='HELP':
            print('Commands are:\r\nSAY: Check laser status\r\nSTART: Start the laser\r\nSTOP: Stop the laser\r\n')
            print('MAX: Set output mode to MAX\r\nADJ: Set output mode to adj\r\n')
            print('Exit application with EXIT command\r\n')
        else:
            if inputmsg == 'MAX':
                inputmsg='E0/S2'
            if inputmsg == 'ADJ':
                inputmsg='E0/S1'
            if inputmsg == 'OFF':
                inputmsg='E0/S0'
            msg="[NL:"+inputmsg+"\PC]\n"
            byte_msg=msg.encode('utf-8')
            ser.write(byte_msg)
            time.sleep(1)
            ser_in=[]
            
            # Reading incoming data from serial port
            while ser.inWaiting() > 0:
                ser_in.append(ser.read(1))
                
            # Converting byte data to string
            out=""
            for i in ser_in:
                si=i.decode("utf-8")
                out+=si
                
            # Extracting actual message
            try:
                out=(out.split(':'))[1].split('\\')[0]
            except:
                out=out
            if out != '':
                print("LASER >> " + out)
finally:
    ser.close()
    exit()