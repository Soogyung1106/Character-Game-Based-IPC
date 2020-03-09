/* initsem -- initailizing semaphore */

#include "semaphore.h"

int initsem(key_t semkey) {
	int status = 0, semid;

	if ((semid = semget(semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL)) == -1) {
		if (errno == EEXIST)
			semid = semget(semkey, 1, 0);
	}
	else /* if created ... */
	{
		semun arg;
		arg.val = 1;
		status = semctl(semid, 0, SETVAL, arg);
	}


	if (semid == -1 || status == -1) {
		perror("initsem failed");
		return (-1);
	}
	/* everything is well done */


	return (semid);

}

