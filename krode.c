/*
 * krode - delete onboard recordings from Rode Interview PRO on Linux
 *
 * Usage: krode delete
 *
 * Reverse-engineered from USB HID traffic capture.
 * Delete command: Report ID 0x01, command byte 0x4A, param 0x01
 *
 * Build: gcc -o krode krode.c -lhidapi-hidraw
 * Deps:  libhidapi-dev (Debian/Ubuntu), hidapi-devel (Fedora)
 *
 * License: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>
#include <hidapi/hidapi.h>

#define RODE_VID        0x19F7

/* Both TX units seen on Interview PRO */
#define RODE_PID_TX1    0x0063
#define RODE_PID_TX2    0x0068

#define REPORT_SIZE     17
#define CMD_DELETE      0x4A
#define DELETE_ALL      0x01

static int delete_recordings(hid_device *dev, unsigned short pid)
{
	unsigned char buf[REPORT_SIZE];
	int ret;

	memset(buf, 0, sizeof(buf));
	buf[0] = 0x01;		/* Report ID */
	buf[1] = CMD_DELETE;	/* 0x4A - delete command */
	buf[2] = DELETE_ALL;	/* 0x01 - delete all */

	printf("Sending delete command to PID 0x%04X...\n", pid);

	ret = hid_write(dev, buf, sizeof(buf));
	if (ret < 0) {
		fprintf(stderr, "hid_write failed: %ls\n", hid_error(dev));
		return -1;
	}

	/* Read response - device ACKs with 0x4A 0x41 0x64 */
	memset(buf, 0, sizeof(buf));
	ret = hid_read_timeout(dev, buf, sizeof(buf), 3000);
	if (ret < 0) {
		fprintf(stderr, "hid_read failed: %ls\n", hid_error(dev));
		return -1;
	}
	if (ret == 0) {
		fprintf(stderr, "Timeout waiting for device response.\n");
		return -1;
	}

	if (buf[1] == CMD_DELETE && buf[2] == 0x41) {
		printf("Done. Recordings deleted (status: %d).\n", buf[3]);
		return 0;
	}

	/* Print raw response for debugging */
	printf("Response: ");
	for (int i = 0; i < ret; i++)
		printf("%02x ", buf[i]);
	printf("\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int found = 0;
	struct hid_device_info *devices, *dev_info;

	if (argc < 2 || strcmp(argv[1], "delete") != 0) {
		fprintf(stderr, "Usage: %s delete\n", argv[0]);
		fprintf(stderr, "  delete  - delete all onboard recordings\n");
		return 1;
	}

	if (hid_init() != 0) {
		fprintf(stderr, "hid_init failed\n");
		return 1;
	}

	devices = hid_enumerate(RODE_VID, 0);
	for (dev_info = devices; dev_info; dev_info = dev_info->next) {
		hid_device *dev = hid_open_path(dev_info->path);
		if (!dev)
			continue;
		delete_recordings(dev, dev_info->product_id);
		hid_close(dev);
		found++;
	}
	hid_free_enumeration(devices);

	if (found == 0) {
		fprintf(stderr,
		    "No Rode Interview PRO found. Check USB connection.\n");
		fprintf(stderr,
		    "You may need to run as root or add a udev rule.\n");
		hid_exit();
		return 1;
	}

	hid_exit();
	return 0;
}
