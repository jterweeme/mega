#include "zd.h"
#include <string.h>

// Used by `getNextPathComponent`
#define MAX_COMPONENT_LEN 12 // What is max length?
#define PATH_COMPONENT_BUFFER_LEN MAX_COMPONENT_LEN+1

bool getNextPathComponent2(char *path, unsigned int *p_offset, char *buffer)
{
    int bufferOffset = 0;
    int offset = *p_offset;

    // Skip root or other separator
    if (path[offset] == '/')
        offset++;
  
    // Copy the next next path segment
    while (bufferOffset < MAX_COMPONENT_LEN && (path[offset] != '/') && (path[offset] != '\0'))
        buffer[bufferOffset++] = path[offset++];

    buffer[bufferOffset] = '\0';

    // Skip trailing separator so we can determine if this
    // is the last component in the path or not.
    if (path[offset] == '/')
        offset++;

    *p_offset = offset;
    return (path[offset] != '\0');
}



bool walkPath2(char *filepath, SdFile& parentDir,
		 bool(*callback)(SdFile& parentDir, char *filePathComponent,
				     bool isLastComponent, void *object), void *object = NULL)
{
    SdFile subfile1;
    SdFile subfile2;
    char buffer[PATH_COMPONENT_BUFFER_LEN]; 
    unsigned int offset = 0;
    SdFile *p_parent;
    SdFile *p_child;
    SdFile *p_tmp_sdfile;  
    p_child = &subfile1;
    p_parent = &parentDir;

    while (true)
    {
        bool moreComponents = getNextPathComponent2(filepath, &offset, buffer);
        bool shouldContinue = callback((*p_parent), buffer, !moreComponents, object);

        if (!shouldContinue) {
      // TODO: Don't repeat this code?
      // If it's one we've created then we
      // don't need the parent handle anymore.
      if (p_parent != &parentDir) {
        (*p_parent).close();
      }
      return false;
        }
    
        if (!moreComponents) {
            break;
        }
    
    bool exists = (*p_child).open(*p_parent, buffer, O_RDONLY);

    if (p_parent != &parentDir) {
      (*p_parent).close();
    }
    
    // Handle case when it doesn't exist and we can't continue...
    if (exists) {
      // We alternate between two file handles as we go down
      // the path.
      if (p_parent == &parentDir) {
        p_parent = &subfile2;
      }

      p_tmp_sdfile = p_parent;
      p_parent = p_child;
      p_child = p_tmp_sdfile;
    } else {
      return false;
    }
  }
  
  if (p_parent != &parentDir) {
    (*p_parent).close(); // TODO: Return/ handle different?
  }

  return true;
}



/*

   The callbacks used to implement various functionality follow.

   Each callback is supplied with a parent directory handle,
   character string with the name of the current file path component,
   a flag indicating if this component is the last in the path and
   a pointer to an arbitrary object used for context.

 */

bool callback_pathExists(SdFile& parentDir, char *filePathComponent, 
			    bool isLastComponent, void *object)
{
  /*

    Callback used to determine if a file/directory exists in parent
    directory.

    Returns true if file path exists.

  */
  SdFile child;

  bool exists = child.open(parentDir, filePathComponent, O_RDONLY);
  
  if (exists) {
     child.close(); 
  }
  
  return exists;
}



bool callback_makeDirPath(SdFile& parentDir, char *filePathComponent, 
			     bool isLastComponent, void *object)
{
    bool result = false;
    SdFile child;
    result = callback_pathExists(parentDir, filePathComponent, isLastComponent, object);

    if (!result)
        result = child.makeDir(parentDir, filePathComponent);
  
    return result;
}

bool callback_remove(SdFile& parentDir, char *filePathComponent, 
			bool isLastComponent, void *object)
{
    if (isLastComponent)
        return SdFile::remove(parentDir, filePathComponent);
    
    return true;
}

bool callback_rmdir(SdFile& parentDir, char *filePathComponent, 
			bool isLastComponent, void *object)
{
    if (isLastComponent)
    {
        SdFile f;

        if (!f.open(parentDir, filePathComponent, O_READ))
            return false;

        return f.rmDir();
    }
    return true;
}

bool ZD::begin(uint8_t csPin)
{
    return card.init(SPI_HALF_SPEED, csPin) &&
         volume.init(card) && root.openRoot(volume);
}


SdFile ZD::getParentDir(const char *filepath, int *index) {
  // get parent directory
  SdFile d1 = root; // start with the mostparent, root!
  SdFile d2;

  // we'll use the pointers to swap between the two objects
  SdFile *parent = &d1;
  SdFile *subdir = &d2;
  
  const char *origpath = filepath;

  while (strchr(filepath, '/')) {

    // get rid of leading /'s
    if (filepath[0] == '/') {
      filepath++;
      continue;
    }
    
    if (! strchr(filepath, '/')) {
      // it was in the root directory, so leave now
      break;
    }

    // extract just the name of the next subdirectory
    uint8_t idx = strchr(filepath, '/') - filepath;
    if (idx > 12)
      idx = 12;    // dont let them specify long names
    char subdirname[13];
    strncpy(subdirname, filepath, idx);
    subdirname[idx] = 0;

    // close the subdir (we reuse them) if open
    subdir->close();
    if (! subdir->open(parent, subdirname, O_READ)) {
      // failed to open one of the subdirectories
      return SdFile();
    }
    // move forward to the next subdirectory
    filepath += idx;

    // we reuse the objects, close it.
    parent->close();

    // swap the pointers
    SdFile *t = parent;
    parent = subdir;
    subdir = t;
  }

  *index = (int)(filepath - origpath);
  // parent is now the parent diretory of the file!
  return *parent;
}


Fyle ZD::open(const char *filepath, uint8_t mode)
{
    int pathidx;
    SdFile parentdir = getParentDir(filepath, &pathidx);
    filepath += pathidx;

    if (!filepath[0])
        return Fyle(parentdir, "/");

    SdFile file;

    if (!parentdir.isOpen())
        return Fyle();

    if (parentdir.isRoot())
    {
        if (!file.open(this->root, filepath, mode))     // verandering hier (this)
            return Fyle();
    }
    else
    {
        if ( ! file.open(parentdir, filepath, mode)) {
            return Fyle();
        }
        // close the parent
        parentdir.close();
    }

    if (mode & (O_APPEND | O_WRITE)) 
        file.seekSet(file.fileSize());

    return Fyle(file, filepath);
}

bool ZD::exists(char *filepath)
{
    return walkPath2(filepath, root, callback_pathExists);
}

bool ZD::mkdir(char *filepath)
{
    return walkPath2(filepath, root, callback_makeDirPath);
}

bool ZD::rmdir(char *filepath)
{
    return walkPath2(filepath, root, callback_rmdir);
}

bool ZD::remove(char *filepath)
{
    return walkPath2(filepath, root, callback_remove);
}

Fyle Fyle::openNextFile(uint8_t mode)
{
  dir_t p;

  //Serial.print("\t\treading dir...");
  while (_file->readDir(&p) > 0) {

    // done if past last used entry
    if (p.name[0] == DIR_NAME_FREE) {
      //Serial.println("end");
      return Fyle();
    }

    // skip deleted entry and entries for . and  ..
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') {
      //Serial.println("dots");
      continue;
    }

    // only list subdirectories and files
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) {
      //Serial.println("notafile");
      continue;
    }

    // print file name with possible blank fill
    SdFile f;
    char name[13];
    _file->dirName(p, name);
    //Serial.print("try to open file ");
    //Serial.println(name);

    if (f.open(_file, name, mode)) {
      //Serial.println("OK!");
      return Fyle(f, name);    
    } else {
      //Serial.println("ugh");
      return Fyle();
    }
  }

  //Serial.println("nothing");
  return Fyle();
}

void Fyle::rewindDirectory()
{  
    if (isDirectory())
        _file->rewind();
}



