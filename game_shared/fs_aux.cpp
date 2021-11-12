#include "fs_aux.h"
#include "cpp_aux.h"
#include <Windows.h>
#include <filesystem>
#include <assert.h>

#ifdef CLIENT_DLL
#include "wrect.h"
#include "cl_dll.h"
extern cl_enginefunc_t gEngfuncs;
#else
#include	"extdll.h"
#include	"util.h"
#endif

void FillTCHAR( TCHAR *tchar, const std::string &str ) {
	strcpy(tchar, str.c_str());
	tchar[str.size() + 1] = NULL;
}

void FS_RemoveDirectory( const std::string &path ) {

	TCHAR pathRaw[MAX_PATH];
	FillTCHAR( pathRaw, path );

	SHFILEOPSTRUCT s = { 0 };
	s.hwnd = NULL;
	s.wFunc = FO_DELETE;
	s.pFrom = pathRaw;
	s.pTo = "";
	s.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFileOperation(&s);
}

void FS_CopyDirectory( const std::string &from, const std::string &to ) {

	TCHAR toRaw[MAX_PATH];
	FillTCHAR( toRaw, to );

	TCHAR fromRaw[MAX_PATH];
	FillTCHAR( fromRaw, from );

	SHFILEOPSTRUCT s2 = { 0 };
	s2.hwnd = NULL;
	s2.wFunc = FO_COPY;
	s2.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMMKDIR;
	s2.pFrom = fromRaw;
	s2.pTo = toRaw;
	SHFileOperation(&s2);
}

const char * FS_GetModDirectoryName() {
#ifdef CLIENT_DLL
	auto modDirectory = gEngfuncs.pfnGetGameDirectory();
#else
	static char modDirectory[MAX_PATH];
	g_engfuncs.pfnGetGameDir( modDirectory );
#endif

	return modDirectory;
}

std::string FS_ResolveModPath( const std::string &path ) {
	namespace fs = std::filesystem;

	assert( aux::str::startsWith( path, "\\" ) == false );

	auto moduleHandle = GetModuleHandle( "hl.exe" );

	auto modDirectory = FS_GetModDirectoryName();
	char modulePath[MAX_PATH];
	GetModuleFileName( moduleHandle, modulePath, MAX_PATH );

	auto modPath = fs::path( modulePath )
		.append( "../" )
		.append( modDirectory );

	return fs::canonical( modPath )
		.append( path )
		.make_preferred()
		.string();
}

std::vector<std::string> FS_GetAllFilesInDirectory( const char *path, const char *extension, bool includeDirectories, bool excludeFiles ) {
	std::vector<std::string> result;

	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	char sPath[2048];
	sprintf( sPath, "%s\\*.*", path );

	if ( ( hFind = FindFirstFile( sPath, &fdFile ) ) == INVALID_HANDLE_VALUE ) {
		return std::vector<std::string>();
	}

	do {
		if ( strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {

			sprintf( sPath, "%s\\%s", path, fdFile.cFileName );

			if ( fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				std::vector<std::string> sub = FS_GetAllFilesInDirectory( sPath, extension, includeDirectories, excludeFiles );
				if ( includeDirectories ) {
					result.push_back( sPath );
				}
				result.insert( result.end(), sub.begin(), sub.end() );
			} else {
				if (
					!excludeFiles &&
					( extension == "*" || aux::str::endsWith( sPath, extension ) )
				) {
					result.push_back( sPath );
				}
			}
		}
	} while ( FindNextFile( hFind, &fdFile ) );

	FindClose( hFind );

	return result;
}

// Based on this answer
// http://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows