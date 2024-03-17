# import the library
from appJar import gui
from array import array
import subprocess
import socket
import sys
import os
import time
import struct

def float_to_hex(f):
    return hex(struct.unpack('<I', struct.pack('<f', f))[0])
def float_to_int(f):
    return int(struct.unpack('<I', struct.pack('<f', f))[0])
def int_to_float(i):
    return float(struct.unpack('<f', struct.pack('<I',i))[0])

def testPulseRange(tp):
  value = app.getEntry(tp)
  if value != None:
    if value < 0:
      value = 0
    elif value>255:
      value = 255
    app.setEntry(tp,int(value))

def VerifyRanges(tp):
#Hibernation Voltages
  voltl = app.getEntry("BatLow")
  if voltl == None: voltl = 5
  if voltl < 5: voltl = 5
  volth = app.getEntry("BatHigh")
  if volth == None: volth = 15
  if volth > 15: volth = 15
  if voltl > (volth - 1):
    if voltl < 7: voltl = 7
    if voltl > 12: voltl = 12
    volth = voltl + 1
  app.setEntry("BatLow",voltl)
  app.setEntry("BatHigh",volth)
# Temperature ranges
  templ = app.getEntry("TempLow")
  if templ == None: templ = 50.0
  temph = app.getEntry("TempHigh")
  if temph == None: temph = 58.0
  if temph > 85: temph = 85
  if templ < 45: templ = 45
  if templ > (temph -2): templ = temph-2
  # should add consistency checks for the temperature values !!!!!!!!!!
  app.setEntry("TempLow",templ)
  app.setEntry("TempHigh",temph)
  for ch in range(1,5):
    GaindB = app.getEntry("C"+str(ch)+"Gain")
    if GaindB < -14: GaindB = -14
    if GaindB > 23.5: GaindB = 23.5
    app.setEntry("C"+str(ch)+"Gain",GaindB)
#readout windows
  pre_time =int(app.getEntry("TPre")/4)
  if pre_time > (1<<12 - 1):
    pre_time = (1<<12 - 1)
  app.setEntry("TPre",int(4*pre_time))
  post_time =int(app.getEntry("TPost")/4)
  if post_time > (1<<12 - 1):
    post_time = (1<<12 - 1)
  app.setEntry("TPost",int(4*post_time))
  overl_time =int(app.getEntry("Tover")/4)
  if overl_time > (1<<5 - 1):
    overl_time = (1<<5 - 1)
  app.setEntry("Tover",int(4*overl_time))
#trigger rate
  internal = 0
  rate = 0
  boxlist = app.getOptionBox("Trigger Source")
  for conf,tick in boxlist.items():
    if tick == True:
      if conf == "Internal":
        internal = 1
  if internal == 1:
    if int(app.getEntry("Test Pulse")) != 0:
      rate = int(app.getEntry("Test Pulse"))
      if rate > 1000000:
        rate = 1000000
      if rate<2:
        rate = 2
      sample_pair = int(2.5e8/rate)>>8;
      sp = sample_pair
      msb = 0;
      while sp != 0:
        sp = int(sp/2)
        msb = msb+1
      if msb>3:
        msb -=4
      else:
        msb = 0
      if msb>15:
        msb = 15
        sample_pair = 15
      else:
        sample_pair = int(sample_pair)>>msb
      if sample_pair == 0:
        sample_pair = 1
      sp = sample_pair<<msb
      rate = 2.5e8/(sp<<8)
  app.setEntry("Test Pulse",int(rate))
# Other channel dependent parameters
  for ch in range(1,4):
    TrigT1 = int(app.getEntry("C"+str(ch)+"TrigT1"))
    if TrigT1>(1<<12-1):
      TrigT1 = 1<<12-1
    app.setEntry("C"+str(ch)+"TrigT1",TrigT1)
    TrigT2 = int(app.getEntry("C"+str(ch)+"TrigT2"))
    if TrigT2>(1<<12-1):
      TrigT2 = 1<<12-1
    app.setEntry("C"+str(ch)+"TrigT2",TrigT2)
    value = app.getEntry("C"+str(ch)+"TrigTprev")
    if value != None:
      value = int(value/4)
      if value < 0:
        value = 0
      elif value > ((1<<9)-1):
        value = ((1<<9)-1)
    else:
      value = 0
    app.setEntry("C"+str(ch)+"TrigTprev",int(4*value))
    value = app.getEntry("C"+str(ch)+"TrigTper")
    if value != None:
      value = int(value/4)
      if value < 0:
        value = 0
      elif value > ((1<<9)-1):
        value = ((1<<9)-1)
    else:
      value = 0
    app.setEntry("C"+str(ch)+"TrigTper",int(4*value))
    value = app.getEntry("C"+str(ch)+"TrigTcmax")
    if value != None:
      value = int(value/4)
      if value < 0:
        value = 0
      elif value > 7:
        value = 7
    else:
      value = 0
    app.setEntry("C"+str(ch)+"TrigTcmax",int(4*value))
    value = app.getEntry("C"+str(ch)+"TrigNcmin")
    if value != None:
      if value < 0:
        value = 0
      elif value > 15:
        value = 15
    else:
      value = 0
    app.setEntry("C"+str(ch)+"TrigNcmin",int(value))
    value = app.getEntry("C"+str(ch)+"TrigNcmax")
    vmin = app.getEntry("C"+str(ch)+"TrigNcmin")
    if value != None:
      if value < vmin:
        value = vmin
      elif value > 31:
        value = 31
    else:
      value = vmin
    app.setEntry("C"+str(ch)+"TrigNcmax",int(value))
    Intt = app.getEntry("C"+str(ch)+"Baseline Averaging")
    if Intt != None:
      if Intt < 0:
        Intt = 0
      elif Intt > 5:
        Intt = 5
    else:
      Intt = 0
    app.setEntry("C"+str(ch)+"Baseline Averaging",int(Intt))


def send_daq(msgid):
    msg = array('I',[5,2,msgid,0x4541,0x4152]) # h = signed short, H = unsigned short
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        # Connect to server and send data
        sock.connect(("localhost", 5010))# start the GUI
#        print(msg,"\n")
        sock.sendall(msg)
        sock.close();

def press(button):
    if button == "Initialize":
        #os.system("killall Adaq")
        #os.system("Adaq/Adaq &")
        #time.sleep(1)
        send_daq(402)
    elif button == "Start run":#
        send_daq(403)
    elif button == "Stop run":
        #start comms
        send_daq(404)
    elif button == "Configure CDAQ":
        app.showSubWindow("DAQ Configuration")
    elif button == "Configure DU":
        app.showSubWindow("DigitalModule")
    elif button == "Blue":
        if(app.getButton("Blue") == "Blue"):
          app.setBg("Blue");
          app.setButton("Blue","Black")
        else:
          app.setBg("Black");
          app.setButton("Blue","Blue")

    else:
        app.stop()

def confbut(button):
  if button == "Read DAQ Configuration file":
    statlist=[]
    ticklist=[]
    fname = app.getEntry("conffile")
    fp=open(fname,"r")
    for line in fp:
      words = line.split()
      if words[0] == "DU":
#        print(words[1])
        statlist.append(words[1]+" " +words[2])
        ticklist.append(True)
      elif words[0] == "#DU":
        statlist.append(words[1]+" " +words[2])
        ticklist.append(False)
      elif words[0] == "EBRUN":
        print(words[1])
        app.setEntry("EBRUN",words[1])
      elif words[0] == "EBMODE":
        if words[1] == "0":
          app.setOptionBox("EBMODE","Physics",True)
        else:
          app.setOptionBox("EBMODE","Test",True)
      elif words[0] == "EBSIZE":
        app.setEntry("EBSIZE",words[1])
      elif words[0] == "EBDIR":
        app.setEntry("EBDIR",words[1])
        app.setEntryWidth("EBDIR",len(app.getEntry("EBDIR")))
      elif words[0] == "EBSITE":
        app.setEntry("EBSITE",words[1])
        app.setEntryWidth("EBSITE",len(app.getEntry("EBSITE")))
      elif words[0] == "EBEXTRA":
        app.setEntry("EBEXTRA",words[1])
        app.setEntryWidth("EBEXTRA",len(app.getEntry("EBEXTRA")))
      elif words[0] == "T3RAND":
        app.setEntry("T3RAND",words[1])
      elif words[0] == "T3STAT":
        app.setEntry("T3STAT",words[1])
      elif words[0] == "T3TIME":
        app.setEntry("T3TIME",words[1])
#      else :
#        print(words[0])
    fp.close()
    app.changeOptionBox("DUs",statlist)
    index=0
    for tick in ticklist:
      app.setOptionBox("DUs",statlist[index],tick)
      index +=1
  elif button == "Write DAQ Configuration file":
    fname = app.getEntry("conffile")
    print("Trying to write to "+fname)
    fp=open(fname,"w")
    fp.write("#Automatically generated from Gui\n")
    boxlist=app.getOptionBox("DUs")
    for stat,tick in boxlist.items():
      if tick == True:
        fp.write("DU "+stat+"\n")
      else:
        fp.write("#DU "+stat+"\n")
    fp.write("EBRUN "+str(int(app.getEntry("EBRUN")))+"\n")
    if(app.getOptionBox("EBMODE") == "Physics"):
      fp.write("EBMODE 0\n")
    else:
      fp.write("EBMODE 2\n")
    fp.write("EBSIZE "+str(int(app.getEntry("EBSIZE")))+"\n")
    fp.write("EBDIR "+app.getEntry("EBDIR")+"\n")
    fp.write("EBSITE "+app.getEntry("EBSITE")+"\n")
    fp.write("EBEXTRA "+app.getEntry("EBEXTRA")+"\n")
    fp.write("T3RAND "+str(int(app.getEntry("T3RAND")))+"\n")
    fp.write("T3STAT "+str(int(app.getEntry("T3STAT")))+"\n")
    fp.write("T3TIME "+str(int(app.getEntry("T3TIME")))+"\n")
    fp.close();
  elif button == "Add DU":
    statlist=[]
    ticklist=[]
    boxlist=app.getOptionBox("DUs")
    for stat,tick in boxlist.items():
      statlist.append(stat)
      ticklist.append(tick)
      statlist.append(app.getEntry("IP adress")+" "+app.getEntry("Port"))
      ticklist.append(True)
      app.changeOptionBox("DUs",statlist)
      index=0
      for tick in ticklist:
        print(tick)
        app.setOptionBox("DUs",statlist[index],tick)
        index +=1
  elif button == "Remove DU":
    statlist=[]
    ticklist=[]
    boxlist=app.getOptionBox("DUs")
    for stat,tick in boxlist.items():
      if stat != app.getEntry("IP adress")+" "+app.getEntry("Port"):
        statlist.append(stat)
        ticklist.append(tick)
        app.changeOptionBox("DUs",statlist)
        index=0
        for tick in ticklist:
          print(tick)
          app.setOptionBox("DUs",statlist[index],tick)
          index +=1

def DUfile(button):
  if button == "Read DU Configuration file":
    print("Reading")
    chval = 0
    fname = app.getEntry("DUconffile")
    fp=open(fname,"r")
    for line in fp:
      if line[:1] != '#':
        words = line.split()
        try:
          axi = int(words[0])
          address = int(words[1],0)
          if address < 0x64:
            value =int(words[2],0)
          if address == 0:
            vh = value>>16;
            volth = (int(100*(vh*2.5*(18+91)/(18*4096))))/100
            vl = value&0xffff;
            voltl = (int(100*(vl*2.5*(18+91)/(18*4096))))/100
            app.setEntry("BatLow",voltl)
            app.setEntry("BatHigh",volth)
          elif address == 4:
            th = value>>16;
            tl = value&0xffff;
            temphA = (int(100*((th-819)/2.654+25)))/100
            templA = (int(100*((tl-819)/2.654+25)))/100
            temphN = int(100*(((2500/4096)*th-400)/19.5))/100
            templN = int(100*(((2500/4096)*tl-400)/19.5))/100
          elif address == 8:
            temphG = int_to_float(value)
          elif address == 0xc:
            templG = int_to_float(value)
          elif address == 0x4c:
            temp_source = value&0x7
          elif address == 0x54:
            vh = value>>16
            vl = value&0xffff
            G1 = int(100*((2.5*37.5*(vh-0.5)/4096-14)))/100
            G2 = int(100*((2.5*37.5*(vl-0.5)/4096-14)))/100
            app.setEntry("C1Gain",G1)
            app.setEntry("C2Gain",G2)
          elif address == 0x58:
            vh = value>>16
            vl = value&0xffff
            G1 = int(100*((2.5*37.5*(vh-0.5)/4096-14)))/100
            G2 = int(100*((2.5*37.5*(vl-0.5)/4096-14)))/100
            app.setEntry("C3Gain",G1)
            app.setEntry("C4Gain",G2)
          elif address == 0x10:
            overl_time = 4*(value&0x1f)
            pre_time = 4*((value>>5)&0xfff);
            post_time = 4*((value>>17)&0xfff);
            app.setEntry("TPre",pre_time)
            app.setEntry("TPost",post_time)
            app.setEntry("Tover",overl_time)
          elif address == 0x14:
            for ch in range(1,4):
              vc = (value>>(5*(ch-1)))&0x1f;
              if vc == 2:
                app.setOptionBox("Channel "+str(ch),"ADC A")
              if vc == 4:
                app.setOptionBox("Channel "+str(ch),"ADC B")
              if vc == 8:
                app.setOptionBox("Channel "+str(ch),"ADC C")
              if vc == 16:
                app.setOptionBox("Channel "+str(ch),"ADC D")
              if vc == 3:
                app.setOptionBox("Channel "+str(ch),"ADC A Filtered")
              if vc == 5:
                app.setOptionBox("Channel "+str(ch),"ADC B Filtered")
              if vc == 9:
                app.setOptionBox("Channel "+str(ch),"ADC C Filtered")
              if vc == 17:
                app.setOptionBox("Channel "+str(ch),"ADC D Filtered")
          elif address == 0x18:
            if value&0x1 == 1:
              app.setOptionBox("Trigger Source","Channel 1",True)
            if (value>>1)&0x1 == 1:
              app.setOptionBox("Trigger Source","Channel 2",True)
            if (value>>2)&0x1 == 1:
              app.setOptionBox("Trigger Source","Channel 3",True)
            if (value>>4)&0x1 == 1:
              app.setOptionBox("Trigger Source","Ch 1 AND Ch 2",True)
            if (value>>5)&0x1 == 1:
              app.setOptionBox("Trigger Source","Ch 1 AND Ch 2 AND Ch 3",True)
            if (value>>6)&0x1 == 1:
              app.setOptionBox("Trigger Source","Ch 1 AND Ch 2 AND NOT Ch 3",True)
            if (value>>7)&0x1 == 1:
              app.setOptionBox("Trigger Source","20 Hz",True)
            if (value>>8)&0x1 == 1:
             app.setOptionBox("Trigger Source","10 sec",True)
            msb = (value>>9)&0xf;
            sample_pair = (value>>13)&0xf;
            sp = sample_pair<<msb
            if sp != 0:
              rate = 2.5e8/(sp<<8)
            else:
              rate = 0
            app.setEntry("Test Pulse",int(rate))
            if rate>0.5:
              app.setOptionBox("Trigger Source","Internal",True)
            for ch in range(1,4):
              vc = (value>>(17+3*(ch-1)))&0x7;
              app.setOptionBox("IIR C"+str(ch),vc)
          for ch in range(1,4):
            if address == int(16+12*ch):
              app.setEntry("C"+str(ch)+"TrigT1",(value>>12)&0xfff)
              app.setEntry("C"+str(ch)+"TrigT2",(value)&0xfff)
            if address == int(20+12*ch):
              app.setEntry("C"+str(ch)+"TrigNcmax",value&0x1f)
              app.setEntry("C"+str(ch)+"TrigNcmin",(value>>5)&0xf)
              app.setEntry("C"+str(ch)+"TrigTcmax",4*((value>>9)&0x7))
              app.setEntry("C"+str(ch)+"TrigTper",4*((value>>12)&0x1ff))
              app.setEntry("C"+str(ch)+"TrigTprev",4*((value>>21)&0x1ff))
          if address == 0x5c:
            app.setEntry("C1BMax",value&0x3ff)
            app.setEntry("C1Baseline Averaging",(value>>10)&0x7)
            app.setEntry("C2BMax",(value>>13)&0x3ff)
            app.setEntry("C2Baseline Averaging",(value>>23)&0x7)
          if address == 0x60:
            app.setEntry("C3BMax",value&0x3ff)
            app.setEntry("C3Baseline Averaging",(value>>10)&0x7)
          for ch in range(1,4):
            for flt in range(1,5):
              if address == int(4+96*int(ch)+24*int(flt-1)):
                app.setEntry("C"+str(ch)+"F"+str(flt)+"M",float(words[2]))
                app.setEntry("C"+str(ch)+"F"+str(flt)+"W",float(words[3]))
        except:
          print("oops "+line)
    fp.close()
    if temp_source == 1:
        app.setOptionBox("Temp. Source","ADC chip")
        app.setEntry("TempLow",templA)
        app.setEntry("TempHigh",temphA)
    elif temp_source  == 2:
        app.setOptionBox("Temp. Source","GPS chip")
        app.setEntry("TempLow",templG)
        app.setEntry("TempHigh",temphG)
    elif temp_source == 4:
        app.setOptionBox("Temp. Source","Nut sensor")
        app.setEntry("TempLow",templN)
        app.setEntry("TempHigh",temphN)
  elif button == "Write DU Configuration file":
    VerifyRanges(button)
    fname = app.getEntry("DUconffile")
    print("Trying to write to "+fname)
    fp=open(fname,"w")
    fp.write("#Automatically generated from Gui\n")
    #Hibernation Voltage ranges
    fp.write("#Hibernation\n")
    voltl = app.getEntry("BatLow")
    volth = app.getEntry("BatHigh")
    value = int(voltl*(18*4096)/(2.5*(18+91)))
    value = value+(int(volth*(18*4096)/(2.5*(18+91)))<<16)
    fp.write("0 0x0000 "+hex(value)+"\n")
    #Temperature Ranges and source
    templ = app.getEntry("TempLow")
    if templ == None: templ = 50
    temph = app.getEntry("TempHigh")
    if temph == None: temph = 55
    tsource =app.getOptionBox("Temp. Source")
    if tsource == "Nut sensor":
      valueh = int(((temph*19.5+400)/1000)*4096/2.5)
      valuel = int(((templ*19.5+400)/1000)*4096/2.5)
    else:
      valueh = int((temph-25)*2.654+819)
      valuel = int((templ-25)*2.654+819)
    fp.write("1 0x004 "+hex((valueh<<16) + valuel)+"\n")
    valueh = float_to_int(float(temph))
    fp.write("2 0x008 "+hex(valueh)+"\n")
    valuel = float_to_int(float(templ))
    fp.write("3 0x00c "+ hex(valuel)+"\n")
    if tsource == "ADC chip":
      fp.write("19 0x04C 0x1 \n")
    elif tsource == "GPS chip":
      fp.write("19 0x04C 0x2 \n")
    elif tsource == "Nut sensor":
      fp.write("19 0x04C 0x4 \n")
    fp.write("#Gain\n")
    GaindB1 = app.getEntry("C1Gain")
    Gain1 = int((4096*(GaindB1+14)/(37.5*2.5))+0.5)
    GaindB2 = app.getEntry("C2Gain")
    Gain2 = int((4096*(GaindB2+14)/(37.5*2.5))+0.5)
    fp.write("21 0x0054 "+hex(int((Gain1<<16)+Gain2))+"\n")
    GaindB3 = app.getEntry("C3Gain")
    Gain3 = int((4096*(GaindB3+14)/(37.5*2.5))+0.5)
    GaindB4 = app.getEntry("C4Gain")
    Gain4 = int((4096*(GaindB4+14)/(37.5*2.5))+0.5)
    fp.write("22 0x0058 "+hex(int((Gain3<<16)+Gain4))+"\n")
     # Trace Length
    pre_time =app.getEntry("TPre")/4
    post_time =app.getEntry("TPost")/4
    overl_time =app.getEntry("Tover")/4
    fp.write("#Trace Length\n")
    fp.write("4 0x010 "+ hex(int((int(post_time)<<17)+(int(pre_time)<<5)+overl_time))+"\n")
    #Channel Readout Source
    fp.write("#Channel Readout Selection\n")
    iselector = 0
    for ch in range(1,4):
      if app.getOptionBox("Channel "+str(ch)) == "ADC A":
        iselector +=(2<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC B":
        iselector +=(4<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC C":
        iselector +=(8<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC D":
        iselector +=(16<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC A Filtered":
        iselector +=(3<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC B Filtered":
        iselector +=(5<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC C Filtered":
        iselector +=(9<<(5*(ch-1)))
      elif app.getOptionBox("Channel "+str(ch)) == "ADC D Filtered":
        iselector +=(17<<(5*(ch-1)))
    fp.write("5 0x014 "+ hex(int(iselector))+"\n")
    fp.write("#Trigger Configuration\n")
    tselect = 0
    internal = 0
    boxlist = app.getOptionBox("Trigger Source")
    for conf,tick in boxlist.items():
      if tick == True:
        if conf == "Channel 1":
          tselect += 1
        elif conf == "Channel 2":
          tselect += (1<<1)
        elif conf == "Channel 3":
          tselect += (1<<2)
        elif conf == "Ch 1 AND Ch 2":
          tselect += (1<<4)
        elif conf == "Ch 1 AND Ch 2 AND Ch 3":
          tselect += (1<<5)
        elif conf == "Ch 1 AND Ch 2 AND NOT Ch 3":
          tselect += (1<<6)
        elif conf == "20 Hz":
          tselect += (1<<7)
        elif conf == "10 sec":
          tselect += (1<<8)
        elif conf == "Internal":
          internal = 1
        else:
          print("Unknown trigger parameter\n")
    if internal == 1:
      if int(app.getEntry("Test Pulse")) != 0:
        rate = int(app.getEntry("Test Pulse"))
        if rate > 1000000:
          rate = 1000000
        if rate<2:
          rate = 2
        sample_pair = int(2.5e8/rate)>>8;
        sp = sample_pair
        msb = 0;
        while sp != 0:
          sp = int(sp/2)
          msb = msb+1
        if msb>3:
          msb -=4
        else:
          msb = 0
        sample_pair = int(sample_pair)>>msb
        if sample_pair == 0:
          sample_pair = 1
        print("Sample pairs = "+str(sample_pair)+" "+str(msb)+"\n");
        tselect = tselect+(int(sample_pair)<<13)+(int(msb)<<9)
    for ch in range(1,4):
      conf = app.getOptionBox("IIR C"+str(ch))
      tselect+=int(conf)<<int(17+3*(ch-1))
    fp.write("6 0x018 "+ hex(int(tselect))+"\n")
    for ch in range(1,4):
      TrigT1 = int(app.getEntry("C"+str(ch)+"TrigT1"))<<12
      TrigT1 += int(app.getEntry("C"+str(ch)+"TrigT2"))
      fp.write(str(int(4+3*ch))+" " +hex(int(16+12*ch))+" "+ hex(int(TrigT1))+"\n")
      TrigCh = int(app.getEntry("C"+str(ch)+"TrigNcmax"))
      TrigCh += int(app.getEntry("C"+str(ch)+"TrigNcmin"))<<5
      TrigCh += int(app.getEntry("C"+str(ch)+"TrigTcmax")/4)<<9
      TrigCh += int(app.getEntry("C"+str(ch)+"TrigTper")/4)<<12
      TrigCh += int(app.getEntry("C"+str(ch)+"TrigTprev")/4)<<21
      fp.write(str(int(5+3*ch))+" "+hex(int(20+12*ch))+" " + hex(int(TrigCh))+"\n")
    fp.write("#Baseline subtraction\n")
    BMax = int(app.getEntry("C1BMax"))
    BMax += int(app.getEntry("C1Baseline Averaging"))<<10
    BMax += int(app.getEntry("C2BMax"))<<13
    BMax += int(app.getEntry("C2Baseline Averaging"))<<23
    fp.write("21 0x05C "+ hex(int(BMax))+"\n")
    BMax = int(app.getEntry("C3BMax"))
    BMax += int(app.getEntry("C3Baseline Averaging"))<<10
    fp.write("22 0x060 "+ hex(int(BMax))+"\n")
    fp.write("#Notch Filters\n")
# Filters to be interpreted by Adaq
    for ch in range(1,4):
      for flt in range(1,5):
        fp.write(str(1+24*ch+6*(flt-1))+" "+hex(4+96*int(ch)+24*int(flt-1))+" ")
        fp.write(str(app.getEntry("C"+str(ch)+"F"+str(flt)+"M"))+" ")
        fp.write(str(app.getEntry("C"+str(ch)+"F"+str(flt)+"W"))+"\n")
    fp.close()
  else:
    print("Oops")

# create a GUI variable called app
app = gui("Login Window", "600x200")
app.setBg("black")
app.setFg("yellow")
app.setFont(12)
# add & configure widgets - widgets get a name, to help referencing them later
app.addLabel("title", "Adaq gui")
#app.setLabelBg("title", "red")
# link the buttons to the function called press
app.addButtons(["Initialize", "Start run","Stop run"], press)
app.addButtons(["Configure CDAQ","Configure DU","Exit gui"], press)
app.addEmptyMessage("EventBuilder")
app.addLabelEntry("Username")
app.addLabelSecretEntry("Password")
#
app.startSubWindow("DAQ Configuration", modal=True)
app.setFg("black")
app.setBg("orange")
app.setSize(800, 300)
row = app.getRow()
app.addButton("Read DAQ Configuration file",confbut,row,0)
app.addEntry("conffile",row,1,2)
fname = os.environ.get("GRANDDAQ_CONF","/")+"Adaq.conf"
app.setEntry("conffile",fname)
app.setEntryWidth("conffile",len(app.getEntry("conffile")))
app.addButton("Write DAQ Configuration file",confbut)
app.addLabelNumericEntry("EBRUN")
app.addLabelOptionBox("EBMODE",["- Mode -","Physics","Test"])
app.setOptionBox("EBMODE","Physics",True)
app.addLabelNumericEntry("EBSIZE")
app.addLabelNumericEntry("T3RAND")
app.addLabelNumericEntry("T3STAT")
app.addLabelNumericEntry("T3TIME")
app.setSticky("w")
app.addLabelEntry("EBDIR",colspan=2)
app.setSticky("w")
app.addLabelEntry("EBSITE",colspan=2)
app.setSticky("w")
app.addLabelEntry("EBEXTRA",colspan=2)
app.setSticky("")
app.addTickOptionBox("DUs","")
row = app.getRow();
app.addLabel("NS","DU",row,0)
app.addLabelEntry("IP adress",row,1)
app.addLabelEntry("Port",row,2)
app.addButton("Add DU",confbut,row,3)
app.addButton("Remove DU",confbut,row,4)
app.stopSubWindow()

#
app.startSubWindow("DigitalModule", modal=True)
app.setFg("black")
app.setBg("LightBlue")
app.setSize(1100, 750)
row = 0
app.addButton("Read DU Configuration file",DUfile,row,0)
app.addEntry("DUconffile",row,1,2)
fname = os.environ.get("GRANDDAQ_CONF","/")+"DU.conf"
app.setEntry("DUconffile",fname)
app.setEntryWidth("DUconffile",len(app.getEntry("DUconffile")))
app.addNamedButton("Verify","VerifyCP",VerifyRanges,row,3)
app.addNamedButton("Exit","DigitalModule",app.hideSubWindow,row,4)
app.addButton("Write DU Configuration file",DUfile)
row = app.getRow()
app.addLabel("Bat","Hibernation Voltage Limits (V)")
app.addNumericEntry("BatLow",row,1)
app.setEntry("BatLow",9.0)
app.addNumericEntry("BatHigh",row,2)
app.setEntry("BatHigh",12.5)
row = row+1
app.addLabel("Temp","Hibernation Temperature Limits (off, on) Celcius")
app.addNumericEntry("TempHigh",row,1)
app.setEntry("TempHigh",58.0)
app.addNumericEntry("TempLow",row,2)
app.setEntry("TempLow",50.0)
app.addLabelOptionBox("Temp. Source",["ADC chip","GPS chip","Nut sensor"],row,3)
app.setOptionBox("Temp. Source","GPS chip",True)
row = row+1
app.addLabel("Gain","Additional Gain (A-D) [-14, 23.5] (dB)",row,0)
for ch in range(1,5):
  app.addNumericEntry("C"+str(ch)+"Gain",row,ch)
  app.setEntry("C"+str(ch)+"Gain",0)
row = row+1
app.addLabel("To","Readout Windows Pre, Post, Overlap (ns)",row,0)
app.addNumericEntry("TPre",row,1)
app.setEntry("TPre",960)
app.addNumericEntry("TPost",row,2)
app.setEntry("TPost",1024)
app.addNumericEntry("Tover",row,3)
app.setEntry("Tover",64)
row = row+1
app.addLabel("TC","Trigger Configuration",row,0)
app.addTickOptionBox("Trigger Source",["Channel 1","Channel 2","Channel 3",
  "Ch 1 AND Ch 2","Ch 1 AND Ch 2 AND Ch 3","Ch 1 AND Ch 2 AND NOT Ch 3", "20 Hz","10 sec","Internal"],row,1)
app.setOptionBox("Trigger Source","Channel 1",True)
app.setOptionBox("Trigger Source","Channel 2",True)
app.setOptionBox("Trigger Source","10 sec",True)
app.addLabel("TP","Internal Trigger Rate [2,1000000] Hz",row,2)
app.addNumericEntry("Test Pulse",row,3)
app.setEntry("Test Pulse",0)
row=row+1
#app.addHorizontalSeparator(row,0,4,colour="red")
#row = row+1
crs,filt_g,trig_T1,trig_T2,trig_Tprev,trig_Tper,trig_Tcmax,trig_Ncmin,trig_Ncmax,row_Int,row_Max=1,2,3,4,5,6,7,8,9,10,11
filt1=17
app.addLabel("CRS","Channel Readout Source",row+crs,0)
app.addLabel("IIR","Number of IIR Filters",row+filt_g,0)
app.addLabel("TrigT1","T1 Threshold (ADC)",row+trig_T1,0)
app.addLabel("TrigT2","T2 Threshold (ADC)",row+trig_T2,0)
app.addLabel("TrigTrev","Quiet Time before T1 threshold (ns)",row+trig_Tprev,0)
app.addLabel("TrigTper","Time after T1 threshold (ns)",row+trig_Tper,0)
app.addLabel("TrigTcmax","Max Time between T2 threshold crossings (ns)",row+trig_Tcmax,0)
app.addLabel("TrigNcmin","Min number of T2 threshold crossings",row+trig_Ncmin,0)
app.addLabel("TrigNcmax","Max number of T2 threshold crossings",row+trig_Ncmax,0)
app.addLabel("Tint","Baseline Integration time (2^x samples)",row+row_Int,0)
app.addLabel("Bmax","Max. Baseline (ADC)",row+row_Max,0)
for flt in range(1,5):
  app.addLabel("Filt"+str(flt),"IIR Notch filter"+str(flt),row+filt1+3*flt,0)
  app.addLabel("F"+str(flt)+"Mean","\t Mean Frequency (MHz)",row+filt1+1+3*flt,0)
  app.addLabel("F"+str(flt)+"Width","\t Pole Radius 1=small, 0=infinite",row+filt1+2+3*flt,0)
for ch in range(1,4):
  app.addOptionBox("Channel "+str(ch),
  ["Off","ADC A","ADC B", "ADC C", "ADC D", "ADC A Filtered","ADC B Filtered","ADC C Filtered","ADC D Filtered"],row+crs,ch)
  app.setOptionBox("Channel "+str(ch),"ADC "+chr(64+ch))
  app.addOptionBox("IIR C"+str(ch),["0","1","2","3","4"],row+filt_g,ch)
  app.setOptionBox("IIR C"+str(ch),"0")
  app.addNumericEntry("C"+str(ch)+"TrigT1",row+trig_T1,ch)
  app.setEntry("C"+str(ch)+"TrigT1",100)
  app.addNumericEntry("C"+str(ch)+"TrigT2",row+trig_T2,ch)
  app.setEntry("C"+str(ch)+"TrigT2",50)
  app.addLabel("C"+str(ch),"Channel "+str(ch),row,ch)
  app.addNumericEntry("C"+str(ch)+"TrigTper",row+trig_Tper,ch)
  app.setEntry("C"+str(ch)+"TrigTper",512)
  app.addNumericEntry("C"+str(ch)+"TrigTprev",row+trig_Tprev,ch)
  app.setEntry("C"+str(ch)+"TrigTprev",512)
  app.addNumericEntry("C"+str(ch)+"TrigTcmax",row+trig_Tcmax,ch)
  app.setEntry("C"+str(ch)+"TrigTcmax",20)
  app.addNumericEntry("C"+str(ch)+"TrigNcmin",row+trig_Ncmin,ch)
  app.setEntry("C"+str(ch)+"TrigNcmin",0)
  app.addNumericEntry("C"+str(ch)+"TrigNcmax",row+trig_Ncmax,ch)
  app.setEntry("C"+str(ch)+"TrigNcmax",10)
  app.addNumericEntry("C"+str(ch)+"Baseline Averaging",row+row_Int,ch)
  app.setEntry("C"+str(ch)+"Baseline Averaging",5)
  app.addNumericEntry("C"+str(ch)+"BMax",row+row_Max,ch)
  app.setEntry("C"+str(ch)+"BMax",512)
  for flt in range(1,5):
    app.addNumericEntry("C"+str(ch)+"F"+str(flt)+"M",row+filt1+1+3*flt,ch)
    app.setEntry("C"+str(ch)+"F"+str(flt)+"M",85+5*flt)
    app.addNumericEntry("C"+str(ch)+"F"+str(flt)+"W",row+filt1+2+3*flt,ch)
    app.setEntry("C"+str(ch)+"F"+str(flt)+"W",0.999)
row = row+34
app.stopSubWindow()
#
app.go()
