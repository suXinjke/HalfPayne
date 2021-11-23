#ifndef FS_AUX_H
#define FS_AUX_H

#include <string>
#include <vector>
#include <set>

void FS_RemoveDirectory( const std::string &path );
void FS_CopyDirectory( const std::string &from, const std::string &to );

void FS_InitModule();
std::vector<std::string> FS_GetAllFilesInDirectory( const char *path, const char *extension = "*", bool includeDirectories = false, bool excludeFiles = false );
std::set<std::string> FS_GetAllFileNamesByWildcard( const char *wildCard );
const char * FS_GetModDirectoryName();
std::string FS_ResolveModPath( const std::string &path );

#endif // FS_AUX_H
