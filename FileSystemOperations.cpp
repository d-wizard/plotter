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
#include "FileSystemOperations.h"
#include "dString.h"
#include "dirent.h"
#include <iostream>
#include <fstream>
#include <algorithm>


#if (defined(_WIN32) || defined(__WIN32__))
   #define WIN_BUILD
   #include <windows.h>
   static const std::string DIR_SEP = "\\";
#else
   #define LINUX_BUILD
   #include <sys/stat.h>
   static const std::string DIR_SEP = "/";
#endif

static const std::string CUR_DIR = ".";
static const std::string UP_DIR = "..";

std::string fso::dirSep()
{
   return DIR_SEP;
}

std::string fso::dirSepToOS(const std::string& t_path)
{
   if(DIR_SEP == "\\")
   {
      return dString::Replace(t_path, "/", "\\");
   }
   else
   {
      return dString::Replace(t_path, "\\", "/");
   }
}

bool fso::ComparePath(tDirListing t_comp1, tDirListing t_comp2)
{
   bool b_retVal = false;

   if( t_comp1.b_isDir == t_comp2.b_isDir)
   {
#ifdef WIN_BUILD
      // Make the paths lower case so the search is not case sensitive.
      t_comp1.t_path = dString::Lower(t_comp1.t_path);
      t_comp2.t_path = dString::Lower(t_comp2.t_path);
#endif
      const char* pc_path1 = t_comp1.t_path.c_str();
      const char* pc_path2 = t_comp2.t_path.c_str();

      // Find first byte that is different
      while(*pc_path1 == *pc_path2)
      {
         ++pc_path1;
         ++pc_path2;
      }

      bool b_diffIsInDir1 = (dString::InStr(pc_path1, DIR_SEP) >= 0);
      bool b_diffIsInDir2 = (dString::InStr(pc_path2, DIR_SEP) >= 0);
      if(*pc_path1 < *pc_path2)
      {
         // Path 1 should come first unless the difference in path 1 is
         // in the file name and the difference in path 2 is in a directory name
         if( !((b_diffIsInDir1 == false) && (b_diffIsInDir2 == true)) )
            b_retVal = true;
      }
      else
      {
         // Path 2 should come first unless the difference in path 2 is
         // in the file name and the difference in path 1 is in a directory name
         if( (b_diffIsInDir1 == true) && (b_diffIsInDir2 == false) )
            b_retVal = true;
      }

      if(b_diffIsInDir1 == b_diffIsInDir2 && 
         (dString::IsStrNum(dString::Left(pc_path1, 1)) || dString::IsStrNum(dString::Left(pc_path2, 1))) )
      {
         std::string t_sameStr = t_comp1.t_path.substr(0, pc_path1 - t_comp1.t_path.c_str());
         std::string t_diffStr1 = pc_path1;
         std::string t_diffStr2 = pc_path2;
         unsigned int i_numCharAtFrontDiffCount1 = 0;
         unsigned int i_numCharAtFrontDiffCount2 = 0;
         unsigned int i_numCharAtBackSameCount = 0;

         while( (i_numCharAtBackSameCount < t_sameStr.size()) &&
                (dString::IsStrNum(t_sameStr.substr(t_sameStr.size() - 1 - i_numCharAtBackSameCount, 1)) == true) )
         {
            ++i_numCharAtBackSameCount;
         }

         while( (i_numCharAtFrontDiffCount1 < t_diffStr1.size()) &&
                (dString::IsStrNum(t_diffStr1.substr(i_numCharAtFrontDiffCount1, 1)) == true) )
         {
            ++i_numCharAtFrontDiffCount1;
         }
         while( (i_numCharAtFrontDiffCount2 < t_diffStr2.size()) &&
                (dString::IsStrNum(t_diffStr2.substr(i_numCharAtFrontDiffCount2, 1)) == true) )
         {
            ++i_numCharAtFrontDiffCount2;
         }

         // Check to see if the beginning of the file name is a number, but 
         // limit to 1 billion, so as to not overflow atoi
         if( ((i_numCharAtFrontDiffCount1+i_numCharAtBackSameCount) > 0) && 
             ((i_numCharAtFrontDiffCount2+i_numCharAtBackSameCount) > 0) &&
             ((i_numCharAtFrontDiffCount1+i_numCharAtBackSameCount) < 10) && 
             ((i_numCharAtFrontDiffCount2+i_numCharAtBackSameCount) < 10) )
         {
            std::string t_numSame = t_sameStr.substr(t_sameStr.size() - i_numCharAtBackSameCount, i_numCharAtBackSameCount);
            std::string t_numStr1 = t_numSame;
            std::string t_numStr2 = t_numSame;
            t_numStr1.append(t_diffStr1.substr(0, i_numCharAtFrontDiffCount1));
            t_numStr2.append(t_diffStr2.substr(0, i_numCharAtFrontDiffCount2));
            unsigned int i_numStr1 = atoi(t_numStr1.c_str());
            unsigned int i_numStr2 = atoi(t_numStr2.c_str());

            if(i_numStr1 < i_numStr2)
               b_retVal = true;
            else if(i_numStr1 > i_numStr2)
               b_retVal = false;

         }
      }
   }
   else if( t_comp1.b_isDir == true && t_comp2.b_isDir == false  )
   {
      b_retVal = true;
   }
   // else path 1 is a file and path 2 is a directory, so return false

   return b_retVal;
}

void fso::GetDirContents(tDirContents& t_dirContents, std::string t_dir, bool b_recursive)
{
   t_dir = dirSepToOS(t_dir);

   DIR* pt_dir;
   struct dirent* pt_dirListing;

   // Make sure the dir ends with the dir seperator.
   t_dir = dString::DontEndWithThis(t_dir, DIR_SEP).append(DIR_SEP);
   pt_dir = opendir(t_dir.c_str());
   if(pt_dir == NULL)
   {
      // Invalid, early return
      closedir(pt_dir);
      return;
   }

   do
   {
      pt_dirListing = readdir(pt_dir);
      if(pt_dirListing == NULL || pt_dirListing->d_name == NULL)
      {
         // Read everything out of the current dir, exit loop.
         break;
      }
      else
      {
         // Make sure the directory listing is not "." or ".."
         if( dString::Compare(pt_dirListing->d_name, CUR_DIR) == false &&
             dString::Compare(pt_dirListing->d_name, UP_DIR ) == false )
         {
            // Create full path to the current directory listing
            std::string t_checkPath = t_dir;
            t_checkPath.append(pt_dirListing->d_name);
            if(DirExists(t_checkPath) == true)
            {
               if(b_recursive == true)
               {
                  // Fill in the contents of the current directory listing.
                  GetDirContents(t_dirContents, t_checkPath, true);
               }
               else
               {
                  // Add current directory listing to the list.
                  tDirListing t_dirListing;
                  t_dirListing.t_path = pt_dirListing->d_name;
                  t_dirListing.b_isDir = true;
                  t_dirContents.push_back(t_dirListing);
               }  

            }
            else
            {
               // Add current directory listing to the list.
               tDirListing t_dirListing;
               
               if(b_recursive == true)
               {
                  t_dirListing.t_path = t_checkPath;
               }
               else
               {
                  t_dirListing.t_path = pt_dirListing->d_name;
               }
               t_dirListing.b_isDir = false;
               t_dirContents.push_back(t_dirListing);
            }
         }
      }
   }
   while(1);
   closedir(pt_dir);
}

bool fso::DirExists(std::string t_dir)
{
   t_dir = dirSepToOS(t_dir);

   bool b_retVal = false;
   DIR* pt_dir;
   struct dirent* pt_dirListing;
   
   t_dir = dString::DontEndWithThis(t_dir, DIR_SEP).append(DIR_SEP);

   pt_dir = opendir(t_dir.c_str());
   if(pt_dir != NULL)
   {
       pt_dirListing = readdir(pt_dir);

       // If it is an existing directory the first listing read will not be NULL
       if( pt_dirListing != NULL && pt_dirListing->d_name != NULL )
       {
          b_retVal = true;
       }
   }
   
   closedir(pt_dir);
   return b_retVal;
}


bool fso::FileExists(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   bool b_retVal = false;

   // Check if there is only a file name, but no path
   if(dString::InStr(t_path, DIR_SEP) < 0)
   {
      std::string t_temp = ".";
      t_temp.append(DIR_SEP).append(t_path);
      t_path = t_temp;
   }

   // Separate the directory and file name
   std::string t_dir = GetDir(t_path).append(DIR_SEP);
   std::string t_file = GetFile(t_path);

   // Verify the directory exists, if the directory doesn't exist, the file won't.
   if( DirExists(t_dir) == true )
   {
      DIR* pt_dir;
      struct dirent* pt_dirListing;

      pt_dir = opendir(t_dir.c_str());
      do
      {
         pt_dirListing = readdir(pt_dir);
         if(pt_dirListing == NULL || pt_dirListing->d_name == NULL)
         {
            // Read all the directory listings in the folder, exit.
            break;
         }
#ifdef WIN_BUILD
         // Check to see if the file names are the same. This is not case sensitive, so it will not work in Linux.
         if( dString::Compare(dString::Lower(pt_dirListing->d_name), dString::Lower(t_file)))
#else
         if( dString::Compare(pt_dirListing->d_name, t_file))
#endif
         {
            // Create the full path to the matching directory listing.
            std::string t_checkPath = t_dir;
            t_checkPath.append(pt_dirListing->d_name);

            // If the potential match is not a directory return true to indicate the file exists.
            b_retVal = !DirExists(t_checkPath);
            break;
         }
      }while(pt_dirListing->d_name != NULL);
      closedir(pt_dir);
   }

   return b_retVal;
}

bool fso::createDir(std::string t_dir)
{
   t_dir = dirSepToOS(t_dir);
#if defined WIN_BUILD
   #ifdef UNICODE
      return (::CreateDirectory(dString::StringToWString(t_dir).c_str(), NULL) != FALSE);
   #else
      return (::CreateDirectory(t_dir.c_str(), NULL) != FALSE);
   #endif
#elif defined LINUX_BUILD
   return mkdir(t_dir.c_str(), 0777) != -1;
#endif
}

bool fso::recursiveCreateDir(std::string t_dir)
{
   t_dir = dirSepToOS(t_dir);

   std::string path = "";
   bool success = true;

   if(dString::Compare(dString::Mid(t_dir, 1, 1),":") == true)
   {
      path = dString::Split(&t_dir, DIR_SEP).append(DIR_SEP);
   }
   else if(dString::Compare(dString::Left(t_dir, 2),"\\\\") == true)
   {
      t_dir = dString::Mid(t_dir, 2);
      path = "\\\\";
      path += (dString::Split(&t_dir, DIR_SEP) + DIR_SEP);
      path += (dString::Split(&t_dir, DIR_SEP) + DIR_SEP);
   }

   if(DirExists(path) == true)
   {
      while(dString::Compare(t_dir, "") == false)
      {
         path.append(dString::Split(&t_dir, DIR_SEP)).append(DIR_SEP);
         if(DirExists(path) == false)
         {
            success = createDir(path);
            if(success == false)
            {
               return false;
            }
         }
      }
   }
   return success;
}

std::string fso::ReadFile(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = "";
   if(FileExists(t_path) == true)
   {
      int length;
      char* buffer;

      std::ifstream is;
      is.open(t_path.c_str(), std::ios::binary );

      // get length of file:
      is.seekg (0, std::ios::end);
      length = is.tellg();

      if(length >= 0)
      {
         is.seekg (0, std::ios::beg);

         // allocate memory:
         buffer = new char [length+1];

         // read data as a block:
         is.read (buffer,length);
         is.close();

         buffer[length] = '\0';
         t_retVal = buffer;

         delete[] buffer;
      }
      else
      {
         is.close();
      }
   }
   return t_retVal;
}

void fso::ReadBinaryFile(std::string t_path, std::vector<char>& t_binaryFile)
{
   t_path = dirSepToOS(t_path);

   if(FileExists(t_path) == true)
   {
      int length;

      std::ifstream is;
      is.open(t_path.c_str(), std::ios::binary );

      // get length of file:
      is.seekg (0, std::ios::end);
      length = is.tellg();

      if(length >= 0)
      {
         is.seekg (0, std::ios::beg);

         // allocate memory:
         t_binaryFile.resize(length);

         // read data as a block:
         is.read (&t_binaryFile[0],length);
         is.close();

      }
      else
      {
         is.close();
      }
   }
}


void fso::WriteFile(std::string t_path, std::string t_fileText)
{
   t_path = dirSepToOS(t_path);

   std::ofstream t_outStream;
   t_outStream.open(t_path.c_str(), std::ios::binary);
   t_outStream.write(t_fileText.c_str(), dString::Len(t_fileText));
   t_outStream.close();
}

void fso::WriteFile(std::string t_path, char* pc_fileText, int i_outSizeBytes)
{
   t_path = dirSepToOS(t_path);

   std::ofstream t_outStream;
   t_outStream.open(t_path.c_str(), std::ios::binary);
   t_outStream.write(pc_fileText, i_outSizeBytes);
   t_outStream.close();
}

void fso::AppendFile(std::string t_path, std::string t_fileText)
{
   t_path = dirSepToOS(t_path);

   std::ofstream t_outStream;
   t_outStream.open(t_path.c_str(), std::ios::binary | std::ios_base::app);
   t_outStream.write(t_fileText.c_str(), dString::Len(t_fileText));
   t_outStream.close();
}

std::string fso::GetDir(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = "";

   int count = dString::Count(t_path, DIR_SEP);
   
   if(count == 1 && dString::Left(t_path, DIR_SEP.length()) == DIR_SEP)
   {
      // Input is in root path, e.g. /file.ext
      t_retVal = "";
   }
   else if(count > 0)
   {
      t_retVal = dString::SplitBackLeft(t_path, DIR_SEP);
   }
   
   return t_retVal;
}

std::string fso::GetFile(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = "";

   if(dString::InStr(t_path, DIR_SEP) >= 0)
   {
      t_retVal = dString::SplitBackRight(t_path, DIR_SEP);
   }
   else
   {
      t_retVal = t_path;
   }
   
   return t_retVal;
}

std::string fso::GetExt(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = "";
   t_retVal = dString::SplitBackRight(t_path, ".");
   if(dString::InStr(t_retVal, DIR_SEP) >= 0)
   {
      t_retVal = "";
   }
   return t_retVal;
}

std::string fso::RemoveExt(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = t_path;
   if(dString::InStrBack(t_retVal, ".") > dString::InStrBack(t_retVal, DIR_SEP))
   {
      t_retVal = dString::SplitBackLeft(t_path, ".");
   }
   return t_retVal;
}

std::string fso::GetFileNameNoExt(std::string t_path)
{
   t_path = dirSepToOS(t_path);

   std::string t_retVal = "";
   t_retVal = GetFile(t_path);
   t_retVal = dString::SplitBackLeft(t_retVal, ".");
   return t_retVal;
}


std::string fso::DontEndWithDirSep(const std::string& t_path)
{
   return dString::DontEndWithThis(dirSepToOS(t_path), DIR_SEP);
}

std::string fso::EndWithDirSep(const std::string& t_path)
{
   return DontEndWithDirSep(t_path) + DIR_SEP;
}


