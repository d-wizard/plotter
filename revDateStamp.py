# Copyright 2015 Dan Williams. All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
import os
import string
import time

SVN_PATH = '"c:/Program Files/TortoiseSVN/bin/svn.exe"'

REV_DELIM = 'Revision: '

FILE_NAME = 'revDateStamp.h'

def readWholeFile(path):
   fileId = open(path, 'r')
   retVal = fileId.read()
   fileId.close()
   return retVal
   
def writeWholeFile(path, fileText):
   fileId = open(path, 'w')
   fileId.write(fileText)
   fileId.close()

def getSvnVersion():
   retVal = 0
   svnVersionCmd = SVN_PATH + ' info'

   p = os.popen(svnVersionCmd, "r")
   while 1:
      line = p.readline()

      if not line: break
      
      if line.find(REV_DELIM) >= 0:
         retVal = int(line.split(REV_DELIM)[1])
         break
   return retVal
       
svnRev = getSvnVersion()
time.ctime()
dateStamp = time.strftime('%m/%d/%y')

revDateInfo = 'Rev: ' + str(svnRev) + " - Date: " + dateStamp

fileText = '#define REV_DATE_STAMP "' + revDateInfo + '"'

if fileText != readWholeFile(FILE_NAME):
   writeWholeFile(FILE_NAME, fileText)