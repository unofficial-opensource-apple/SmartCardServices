/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License. Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 */

/******************************************************************

	MUSCLE SmartCard Development ( http://www.linuxnet.com )
	Title  : sys_unix.c
	Package: pcsc lite
	Author : David Corcoran
	Date   : 11/8/99
	License: Copyright (C) 1999 David Corcoran
			<corcoran@linuxnet.com>
	Purpose: This handles abstract system level calls. 

$Id: sys_macosx.cpp,v 1.5 2004/09/21 02:43:57 mb Exp $

********************************************************************/

#include <sys_generic.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <securityd_client/ssclient.h>
#include <security_utilities/debugging.h>

#include "config.h"


extern "C" {

int SYS_Initialize()
{
	/*
	 * Nothing special for OS X and Linux 
	 */
	return 0;
}

int SYS_Mkdir(char *path, int perms)
{
	return mkdir(path, perms);
}

int SYS_GetPID()
{
	return getpid();
}

int SYS_Sleep(int iTimeVal)
{
	return sleep(iTimeVal);
}

int SYS_USleep(int iTimeVal)
{
	usleep(iTimeVal);
	return iTimeVal;
}

int SYS_OpenFile(char *pcFile, int flags, int mode)
{
	return open(pcFile, flags, mode);
}

int SYS_CloseFile(int iHandle)
{
	return close(iHandle);
}

int SYS_RemoveFile(char *pcFile)
{
	return remove(pcFile);
}

int SYS_Chmod(const char *path, int mode)
{
	return chmod(path, mode);
}

int SYS_Chdir(const char *path)
{
	return chdir(path);
}

int SYS_Mkfifo(const char *path, int mode)
{
	return mkfifo(path, mode);
}

int SYS_Mknod(const char *path, int mode, int dev)
{
	return mknod(path, mode, dev);
}

int SYS_GetUID()
{
	return getuid();
}

int SYS_GetGID()
{
	return getgid();
}

int SYS_Chown(const char *fname, int uid, int gid)
{
	return chown(fname, uid, gid);
}

int SYS_ChangePermissions(char *pcFile, int mode)
{
	return chmod(pcFile, mode);
}

int SYS_LockFile(int iHandle)
{
	return flock(iHandle, LOCK_EX | LOCK_NB);
}

int SYS_LockAndBlock(int iHandle)
{
	return flock(iHandle, LOCK_EX);
}

int SYS_UnlockFile(int iHandle)
{
	return flock(iHandle, LOCK_UN);
}

int SYS_SeekFile(int iHandle, int iSeekLength)
{
	int iOffset;
	iOffset = lseek(iHandle, iSeekLength, SEEK_SET);
	return iOffset;
}

int SYS_ReadFile(int iHandle, char *pcBuffer, int iLength)
{
	return read(iHandle, pcBuffer, iLength);
}

int SYS_WriteFile(int iHandle, char *pcBuffer, int iLength)
{
	return write(iHandle, pcBuffer, iLength);
}

int SYS_GetPageSize(void)
{
	return getpagesize();
}

void *SYS_MemoryMap(int iSize, int iFid, int iOffset)
{

	void *vAddress;

	vAddress = 0;
	vAddress = mmap(0, iSize, PROT_READ | PROT_WRITE,
		MAP_SHARED, iFid, iOffset);

	/*
	 * Here are some common error types: switch( errno ) { case EINVAL:
	 * printf("EINVAL"); case EBADF: printf("EBADF"); break; case EACCES:
	 * printf("EACCES"); break; case EAGAIN: printf("EAGAIN"); break; case 
	 * ENOMEM: printf("ENOMEM"); break; } 
	 */

	return vAddress;
}

void *SYS_PublicMemoryMap(int iSize, int iFid, int iOffset)
{

	void *vAddress;

	vAddress = 0;
	vAddress = mmap(0, iSize, PROT_READ, MAP_SHARED, iFid, iOffset);
	return vAddress;
}

int SYS_MMapSynchronize(void *begin, int length)
{
	int rc = msync(begin, length, MS_SYNC | MS_INVALIDATE);
	
	// send a change notification to securityd
	using namespace SecurityServer;
	ClientSession session(Allocator::standard(), Allocator::standard());
	try {
		session.postNotification(kNotificationDomainPCSC,
			kNotificationPCSCStateChange, CssmData());
		secdebug("pcscd", "notification sent");
	} catch (const MachPlusPlus::Error &err) {
		switch (err.error) {
		case BOOTSTRAP_UNKNOWN_SERVICE: // securityd not yet available; this is not an error
			secdebug("pcscd", "securityd not up; no notification sent");
			break;
#if !defined(NDEBUG)
		// for debugging only, support a securityd restart. This is NOT thread-safe
		case MACH_SEND_INVALID_DEST:
			secdebug("pcscd", "resetting securityd connection for debugging");
			session.reset();
			try {
				session.postNotification(kNotificationDomainPCSC,
					kNotificationPCSCStateChange, CssmData());
			} catch (...) {
				secdebug("pcscd", "re-send attempt failed, punting");
			}
			break;
#endif //NDEBUG
		default:
			secdebug("pcscd", "exception trying to send notification (ignored)");
		}
	} catch (...) {
		secdebug("pcscd", "trouble sending security notification (ignored)");
	}
	return rc;
}

int SYS_Fork()
{
	return fork();
}

#ifdef HAVE_DAEMON
int SYS_Daemon(int nochdir, int noclose)
{
	return daemon(nochdir, noclose);
}
#endif

int SYS_Wait(int iPid, int iWait)
{
	return waitpid(-1, 0, WNOHANG);
}

int SYS_Stat(char *pcFile, struct stat *psStatus)
{
	return stat(pcFile, psStatus);
}

int SYS_Fstat(int iFd)
{
	struct stat sStatus;
	return fstat(iFd, &sStatus);
}

int SYS_Random(int iSeed, float fStart, float fEnd)
{

	int iRandNum = 0;

	if (iSeed != 0)
	{
		srand(iSeed);
	}

	iRandNum = 1 + (int) (fEnd * rand() / (RAND_MAX + fStart));
	srand(iRandNum);

	return iRandNum;
}

int SYS_GetSeed()
{
	struct timeval tv;
	struct timezone tz;
	long myseed = 0;

	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;
	if (gettimeofday(&tv, &tz) == 0)
	{
		myseed = tv.tv_usec;
	} else
	{
		myseed = (long) time(NULL);
	}
	return myseed;
}

void SYS_Exit(int iRetVal)
{
	_exit(iRetVal);
}

int SYS_Rmdir(char *pcFile)
{
	return rmdir(pcFile);
}

int SYS_Unlink(char *pcFile)
{
	return unlink(pcFile);
}

}   // extern "C"
