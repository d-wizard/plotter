/* Copyright 2013 Dan Williams. All Rights Reserved.
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
   else if(i_len <= i_inputLen)
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
   else if(i_len <= i_inputLen)
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
   int i_outputLen = i_inputLen - i_startPos + 1;
   t_retVal = Right(t_inputString, i_outputLen);
   return t_retVal;
}


std::string dString::Mid(const std::string& t_inputString, int i_startPos, int i_len)
{
   std::string t_retVal = t_inputString;
   int i_inputLen = Len(t_inputString);
   int i_outputLen = i_inputLen - i_startPos + 1;

   if(i_len == 0)
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
      memcpy(pc_retVal, t_inputString.c_str()+(i_startPos-1), i_len);
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

   for(i_index = 0; i_index < (i_inputLen - i_searchLen + 1); ++i_index)
   {
      if(memcmp(t_inputString.c_str()+i_index, t_searchString.c_str(), i_searchLen) == 0)
      {
         return i_index+1;
      }
   }

   return 0;
}

int dString::InStrBack(const std::string& t_inputString, const std::string& t_searchString)
{
   int i_inputLen = Len(t_inputString);
   int i_searchLen = Len(t_searchString);
   int i_index;

   for(i_index = (i_inputLen - i_searchLen); i_index >= 0; --i_index)
   {
      if(memcmp(t_inputString.c_str()+i_index, t_searchString.c_str(), i_searchLen) == 0)
      {
         return i_index+1;
      }
   }

   return 0;
}

std::string dString::SplitBackRight(const std::string& t_inputString, const std::string& t_searchString)
{
   int pos;
   pos = InStrBack(t_inputString, t_searchString);
   if(pos > 0)
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
   if(pos > 1)
   {
      return Left(t_inputString, pos - 1);
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
   if(pos > 0)
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
   if(pos > 0)
   {
      return Left(t_inputString, pos - 1);
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
   
   while(InStr(t_in, t_searchString) > 0)
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
      if(pos > 0)
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

   if(Len(t_inputString) > 0)
   {
      b_retVal = true;
      for(i_index = 0; i_index < Len(t_inputString); ++i_index)
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
         t_retVal = Mid(t_retVal, Len(t_searchString)+1);
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
      while(t_inStr != "" && InStr(t_inStr, t_searchString) > 0)
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