#ifndef PATHUTIL_H
#define PATHUTIL_H

bool PathIsStaticLibrary(const char *path);
bool PathIsObjectFile(const char *path);
bool PathIsSignatureFile(const char *path);
bool PathIsN64Rom(const char *path);
size_t PathGetFileName(const char *path, char *dstName, size_t maxLength);
bool IsFileWithSymbols(const char *path);

#endif // PATHUTIL_H
