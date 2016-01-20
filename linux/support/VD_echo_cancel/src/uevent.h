#ifndef U_EVENT_H
#define U_EVENT_H

#include <unistd.h>
#include <signal.h>

#ifdef SUPPORT_RATE_DETECTOR
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

#ifdef  DEBUG
#define	pr_debug(m...)	printf(m)
#else
#define	pr_debug(m...)
#endif

struct udev_message {
    const char *sample_frame;
    int sample_rate;
};

#define AUDIO_UEVENT_MESSAGE	"SAMPLERATE_CHANGED"

__STATIC__ int audio_uevent_init(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int fd;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(0 > fd)
        return -EINVAL;

    setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
    if (0 > bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
        close(fd);
        return -EINVAL;
    }

    return fd;
}

__STATIC__ void audio_uevent_close(int fd)
{
 	close(fd);
}

__STATIC__ int audio_uevent_event(int fd, char* buffer, int length)
{
	struct pollfd fds;
    int n, count;

     do {
    	fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;
        n = poll(&fds, 1, -1);
        if(n > 0 && POLLIN == fds.revents) {
            count = recv(fd, buffer, length, 0);
            if (count > 0)
                return count;
        }
    } while (1);

	// won't get here
    return 0;
}

__STATIC__ void audio_uevent_parse(const char *msg, struct udev_message *udev)
{
	const char *index = NULL;

    udev->sample_frame = "";
    udev->sample_rate = 0;

	/* currently ignoring SEQNUM */
    while(*msg) {

		index = "SAMPLERATE_CHANGED=";
        if(!strncmp(msg, index, strlen(index))) {
            msg += strlen(index);
            udev->sample_rate = strtoll(msg, NULL, 10);
            pr_debug("[%s] %d\n", index, udev->sample_rate);
        }

		index = "SAMPLE_NO_DATA=";
        if(!strncmp(msg, index, strlen(index))) {
            msg += strlen(index);
            udev->sample_frame = msg;
            pr_debug("[%s] %s\n", index, udev->sample_frame);
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }
}
#endif
#endif /* U_EVENT_H */