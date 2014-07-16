/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <string>
#include <algorithm>
#include "dString.h"

#if (defined(_WIN32) || defined(__WIN32__))
   #define WIN_BUILD
   static const std::string LINE_ENDING = "\r\n";
#else
   #define LINUX_BUILD
   static const std::string LINE_ENDING = "\n";
#endif

int dString::Len(const std::string& t_inputString)
{
   return (int)t_inputString.size();
}

std::string dString::Left(const std::string& t_inputString, int i_len)
{
   std::string t_retVal = t_inputString;
   int i_inputLen = Len(t_inputString);
   if(i_len < 1)
   {
      t_retVal = "";
   }
   else if(i_len < i_inputLen)
   {
      char* pc_retVal = new char[i_len+1];
      memcpy(pc_retVal, t_inputString.c_str(), i_len);
      pc_retVal[i_len] = '\0';
      t_retVal = pc_retVal;
      delete[] pc_retVal;
   }
   return t_retVal;
}

std::string dString::Right(const std::string& t_inputString, int i_len)
{
   std::string t_retVal = t_inputString;
   int i_inputLen = Len(t_inputString);
   if(i_len < 1)
   {
      t_retVal = "";
   }
   else if(i_len < i_inputLen)
   {
      char* pc_retVal = new char[i_len+1];
      memcpy(pc_retVal, t_inputString.c_str()+(i_inputLen-i_len), i_len);
      pc_retVal[i_len] = '\0';
      t_retVal = pc_retVal;
      delete[] pc_retVal;
   }
   return t_retVal;
}

std::string dString::Mid(const std::string& t_inputString, int i_startPos)
{
   std::string t_retVal = t_inputString;
   int i_inputLen = Len(t_inputString);
   int i_outputLen = i_inputLen - i_startPos;
   t_retVal = Right(t_inputString, i_outputLen);
   return t_retVal;
}


std::string dString::Mid(const std::string& t_inputString, int i_startPos, int i_len)
{
   int i_inputLen = Len(t_inputString);
   
   // Fix input.
   if(i_startPos < 0)
   {
      i_len += i_startPos;
      i_startPos = 0;
   }
   else if(i_startPos >= i_inputLen)
   {
      return "";
   }

   std::string t_retVal = t_inputString;
   int i_outputLen = i_inputLen - i_startPos;

   if(i_len <= 0)
   {
      t_retVal = "";
   }
   else if(i_len >= i_outputLen)
   {
      t_retVal = Right(t_inputString, i_outputLen);
   }
   else
   {
      char* pc_retVal = new char[i_len+1];
      memcpy(pc_retVal, t_inputString.c_str()+i_startPos, i_len);
      pc_retVal[i_len] = '\0';
      t_retVal = pc_retVal;
      delete[] pc_retVal;
   }
   return t_retVal;
}

int dString::InStr(const std::string& t_inputString, const std::string& t_searchString)
{
   int i_inputLen = Len(t_inputString);
   int i_searchLen = Len(t_searchString);
   int i_index;

   if(t_searchString == "")
      return -1;

   for(i_index = 0; i_index < (i_inputLen - i_searchLen + 1); ++i_index)
   {
      if(memcmp(t_inputString.c_str()+i_index, t_searchString.c_str(), i_searchLen) == 0)
      {
         return i_index;
      }
   }

   return -1;
}

int dString::InStrBack(const std::string& t_inputString, const std::string& t_searchString)
{
   int i_inputLen = Len(t_inputString);
   int i_searchLen = Len(t_searchString);
   int i_index;

   if(t_searchString == "")
      return -1;

   for(i_index = (i_inputLen - i_searchLen); i_index >= 0; --i_index)
   {
      if(memcmp(t_inputString.c_str()+i_index, t_searchString.c_str(), i_searchLen) == 0)
      {
         return i_index;
      }
   }

   return -1;
}

std::string dString::SplitBackRight(const std::string& t_inputString, const std::string& t_searchString)
{
   int pos;
   pos = InStrBack(t_inputString, t_searchString);
   if(pos >= 0)
   {
      return Mid(t_inputString, pos + t_searchString.size());
   }
   else
   {
      return "";
   }
}

std::string dString::SplitBackLeft(const std::string& t_inputString, const std::string& t_searchString)
{
   int pos;
   pos = InStrBack(t_inputString, t_searchString);
   if(pos >= 0)
   {
      return Left(t_inputString, pos);
   }
   else
   {
      return t_inputString;
   }
}

std::string dString::SplitRight(const std::string& t_inputString, const std::string& t_searchString)
{
   int pos;
   pos = InStr(t_inputString, t_searchString);
   if(pos >= 0)
   {
      return Mid(t_inputString, pos + t_searchString.size());
   }
   else
   {
      return "";
   }
}

std::string dString::SplitLeft(const std::string& t_inputString, const std::string& t_searchString)
{
   int pos;
   pos = InStr(t_inputString, t_searchString);
   if(pos >= 0)
   {
      return Left(t_inputString, pos);
   }
   else
   {
      return t_inputString;
   }
}

std::string dString::Replace(const std::string& t_inputString, const std::string& t_searchString, const std::string& t_replaceString)
{
   std::string t_in = t_inputString;
   std::string t_out = "";
   
   while(InStr(t_in, t_searchString) >= 0)
   {
      t_out.append(SplitLeft(t_in, t_searchString));
      t_out.append(t_replaceString);
      t_in = SplitRight(t_in, t_searchString);
   }
   t_out.append(t_in);
   return t_out;
}

std::string dString::Split(std::string* t_inputString, const std::string& t_searchString)
{
   std::string t_retVal;
   if(t_searchString != "")
   {
      int pos;
      pos = InStr(*t_inputString, t_searchString);
      if(pos >= 0)
      {
         t_retVal = SplitLeft(*t_inputString, t_searchString);
         *t_inputString = SplitRight(*t_inputString, t_searchString);
      }
      else
      {
         t_retVal = *t_inputString;
         *t_inputString = "";
      }
   }
   else
   {
      t_retVal = *t_inputString;
      *t_inputString = "";
   }
   return t_retVal;
}

std::string dString::GetMiddle(std::string* t_inputString, const std::string& t_searchStringStart, const std::string& t_searchStringEnd)
{
   std::string t_retVal;
   Split(t_inputString, t_searchStringStart);
   t_retVal = Split(t_inputString, t_searchStringEnd);
   return t_retVal;
}

bool dString::IsStrNum(const std::string& t_inputString)
{
   bool b_retVal = false;
   int i_index = 0;
   const char* pc_inStr = t_inputString.c_str();
   int i_len = Len(t_inputString);

   if(i_len > 0)
   {
      if(*pc_inStr == '-' && i_len > 1)
      {
         pc_inStr++;
         i_len--;
      }
   
      b_retVal = true;
      for(i_index = 0; i_index < i_len; ++i_index)
      {
         if(*pc_inStr < '0' || *pc_inStr > '9')
         {
            b_retVal = false;
            break;
         }
         pc_inStr++;
      }
   }
   return b_retVal;
}

// Returns the beginning of the string until is ceases to be a number.
std::string dString::GetNumFromStr(const std::string& inStr)
{
   const char* cStr = inStr.c_str();
   unsigned int strLen = inStr.size();

   std::string outString = "";

   if(cStr[0] == '-')
   {
      // Negative number.
      outString.append(&cStr[0]);

      // Skip past negative sign.
      ++cStr;
      --strLen;
   }
   for(unsigned int i = 0; i < strLen; ++i)
   {
      std::string curChar(1, cStr[i]);
      if(dString::IsStrNum(curChar))
         outString.append(curChar);
      else
         break;
   }
   return outString;
}

// Returns the beginning of the string until is ceases to be a number
// and removes the number portion from the beginning of inOutStr
std::string dString::SplitNumFromStr(std::string& inOutStr)
{
   std::string retVal = GetNumFromStr(inOutStr);
   int retValLen = retVal.size();
   inOutStr = inOutStr.substr(retValLen, inOutStr.size() - retValLen);
   return retVal;
}

std::string dString::Chr(const unsigned char c_chr)
{
   char pc_chr[2];
   std::string t_retVal;

   pc_chr[0] = c_chr;
   pc_chr[1] = 0;
   t_retVal = pc_chr;
   return t_retVal;
}

unsigned char dString::Asc(const std::string& t_inputString)
{
   unsigned char c_retVal;
   
   if(Len(t_inputString) < 1)
   {
      c_retVal = 0;
   }
   else
   {
      c_retVal = *(t_inputString.c_str());
   }

   return c_retVal;
}


std::string dString::Upper(const std::string& t_inputString)
{
   std::string t_retVal;
   int i_len = Len(t_inputString);
   char* pc_retVal = new char[i_len+1];
   int i_index;

   memcpy(pc_retVal, t_inputString.c_str(), i_len+1);
   for(i_index = 0; i_index < i_len; ++i_index)
   {
      if( (pc_retVal[i_index]) >= 'a' && (pc_retVal[i_index]) <= 'z' )
      {
         (pc_retVal[i_index]) += ('A' - 'a');
      }
   }

   t_retVal = pc_retVal;
   delete[] pc_retVal;
   return t_retVal;
}
std::string dString::Lower(const std::string& t_inputString)
{
   std::string t_retVal;
   int i_len = Len(t_inputString);
   char* pc_retVal = new char[i_len+1];
   int i_index;

   memcpy(pc_retVal, t_inputString.c_str(), i_len+1);
   for(i_index = 0; i_index < i_len; ++i_index)
   {
      if( (pc_retVal[i_index]) >= 'A' && (pc_retVal[i_index]) <= 'Z' )
      {
         (pc_retVal[i_index]) += ('a' - 'A');
      }
   }
   
   t_retVal = pc_retVal;
   delete[] pc_retVal;
   return t_retVal;
}

bool dString::Compare(const char* t_cmp1, const char* t_cmp2)
{
   bool b_retVal = false;
   if( (t_cmp1 != NULL) &&
       (t_cmp2 != NULL) )
   {
      int i_len1 = (int)strlen(t_cmp1);
      int i_len2 = (int)strlen(t_cmp2);
      if( (i_len1 == i_len2) &&
           (memcmp(t_cmp1, t_cmp2, i_len1+1) == 0) )
      {
         b_retVal = true;
      }
   }
   return b_retVal;
}
bool dString::Compare(std::string t_cmp1, std::string t_cmp2)
{
   return Compare(t_cmp1.c_str(), t_cmp2.c_str());
}
bool dString::Compare(const char* t_cmp1, std::string t_cmp2)
{
   return Compare(t_cmp1, t_cmp2.c_str());
}
bool dString::Compare(std::string t_cmp1, const char* t_cmp2)
{
   return Compare(t_cmp1.c_str(), t_cmp2);
}

std::string dString::DontStartWithThis(const std::string& t_inputString, const std::string& t_searchString)
{
   std::string t_retVal = t_inputString;

   if(t_searchString != "")
   {
      while( Left(t_retVal, Len(t_searchString)) == t_searchString)
      {
         t_retVal = Mid(t_retVal, Len(t_searchString));
      }  
   }
   return t_retVal;
}

std::string dString::DontEndWithThis(const std::string& t_inputString, const std::string& t_searchString)
{
   std::string t_retVal = t_inputString;

   if(t_searchString != "")
   {
      while( Right(t_retVal, Len(t_searchString)) == t_searchString)
      {
         t_retVal = Left(t_retVal, Len(t_retVal) - Len(t_searchString));
      }  
   }
   return t_retVal;
}

int dString::Count(const std::string& t_inputString, const std::string& t_searchString)
{
   int i_retVal = 0;
   std::string t_inStr = t_inputString;
   
   if(t_searchString != "")
   {
      while(t_inStr != "" && InStr(t_inStr, t_searchString) >= 0)
      {
         Split(&t_inStr, t_searchString);
         ++i_retVal;
      }
   }

   return i_retVal;
}

std::wstring dString::StringToWString(const std::string& t_inputString)
{
std::wstring temp(t_inputString.length(),L' ');
std::copy(t_inputString.begin(), t_inputString.end(), temp.begin());
return temp;
}


std::string dString::WStringToString(const std::wstring& t_inputString)
{
std::string temp(t_inputString.length(), ' ');
std::copy(t_inputString.begin(), t_inputString.end(), temp.begin());
return temp;
}


std::string dString::GetLineEnding()
{
   return LINE_ENDING;
}
std::string dString::ConvertLineEndingToDos(const std::string& t_text)
{
   return Replace(Replace(t_text, "\r\n", "\n"), "\n", "\r\n");
}
std::string dString::ConvertLineEndingToUnix(const std::string& t_text)
{
   return Replace(t_text, "\r\n", "\n");
}
std::string dString::ConvertLineEndingToOS(const std::string& t_text)
{
#ifdef WIN_BUILD
   return ConvertLineEndingToDos(t_text);
#else
   return ConvertLineEndingToUnix(t_text);
#endif
}

std::string dString::Slice(const std::string& t_inputString, int sliceRHS, int sliceLHS)
{
   std::string retVal = "";
   int strLen = (int)t_inputString.size();

   if(sliceRHS < 0)
      sliceRHS += strLen;
   if(sliceLHS <= 0)
      sliceLHS += strLen;
   
   if(sliceRHS < sliceLHS)
   {
      retVal = Mid(t_inputString, sliceRHS, sliceLHS-sliceRHS);
   }
   
   return retVal;
}

std::vector<std::string> dString::SplitV(const std::string& t_input, const std::string& t_delimiter)
{
   std::string in = t_input;
   std::vector<std::string> retVal;
   
   while(InStr(in, t_delimiter) >= 0)
   {
      retVal.push_back(Split(&in, t_delimiter));
   }
   retVal.push_back(in);
   
   return retVal;
}

std::string dString::JoinV(const std::vector<std::string>& t_input, const std::string& t_delimiter)
{
   std::string retVal("");
   if(t_input.size() > 0)
   {
      std::vector<std::string>::const_iterator lastIter = --t_input.end();
      for(std::vector<std::string>::const_iterator iter = t_input.begin(); iter != lastIter; ++iter)
      {
         retVal += ((*iter) + t_delimiter);
      }
      retVal += (*lastIter);
   }
   return retVal;
}


std::list<std::string> dString::SplitL(const std::string& t_input, const std::string& t_delimiter)
{
   std::string in = t_input;
   std::list<std::string> retVal;
   
   while(InStr(in, t_delimiter) >= 0)
   {
      retVal.push_back(Split(&in, t_delimiter));
   }
   retVal.push_back(in);
   
   return retVal;
}

std::string dString::JoinL(const std::list<std::string>& t_input, const std::string& t_delimiter)
{
   std::string retVal("");
   if(t_input.size() > 0)
   {
      std::list<std::string>::const_iterator lastIter = --t_input.end();
      for(std::list<std::string>::const_iterator iter = t_input.begin(); iter != lastIter; ++iter)
      {
         retVal += ((*iter) + t_delimiter);
      }
      retVal += (*lastIter);
   }
   return retVal;
}




