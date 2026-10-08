/* ReadControllers__Fv (0x80070d10) */
/* C++ (PAD.CPP). Demo twin. Multitap ports (type 0x80) read four controllers; otherwise read one and mark the other three absent. */
typedef unsigned char uchar;

struct ControllerData {
	uchar status;
	uchar type;
	uchar button1;
	uchar button2;
	uchar analog0;
	uchar analog1;
	uchar analog2;
	uchar analog3;
};

struct ControllerBuffer {
	uchar pad[34];
};

extern ControllerBuffer controllers[2];
extern ControllerData theControllers[8];

void ReadController(ControllerData *, uchar *);

void ReadControllers() {
	int controller_port;

	for (controller_port = 0; controller_port < 2; controller_port++) {
		if (controllers[controller_port].pad[1] == 0x80) {
			ReadController(&theControllers[controller_port * 4 + 0],
			               &controllers[controller_port].pad[2]);
			ReadController(&theControllers[controller_port * 4 + 1],
			               &controllers[controller_port].pad[10]);
			ReadController(&theControllers[controller_port * 4 + 2],
			               &controllers[controller_port].pad[18]);
			ReadController(&theControllers[controller_port * 4 + 3],
			               &controllers[controller_port].pad[26]);
		} else {
			ReadController(&theControllers[controller_port * 4], controllers[controller_port].pad);

			theControllers[controller_port * 4 + 1].type = 0;
			theControllers[controller_port * 4 + 2].type = 0;
			theControllers[controller_port * 4 + 3].type = 0;
		}
	}
}
