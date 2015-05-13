#pragma once

#include "../../debug.hpp"
#include "../../kbd_defines.hpp"
#include "../../kbd_handler_prolog.hpp"

namespace kbd_handler {

/*
 * State transitions
 */

static void
on_mode_timeout(App & app, ModeId mode_if_inactive)
{
	ModeId m = mode_if_inactive;
	if (app.drone_status.active())
	{
		m = app.drone_status.in_air() ?
			ModeId::FLIGHT : ModeId::PREFLIGHT;
	}
	app.set_mode_transition(m);
}

template <ModeId MODE>
struct handle< MODE, EventKind::COPTER_CHANGED_STATE, BTN_NONE, When<
	MODE == ModeId::INIT or MODE == ModeId::PREFLIGHT
> > {
	static void
	exec(App & app)
	{
		say("INIT or PREFLIGHT on_mode_timeout");
		on_mode_timeout(app, ModeId::INIT);
	}
};

template <ModeId MODE>
struct handle< MODE, EventKind::COPTER_CHANGED_STATE, BTN_NONE, When<
	in_air_mode(MODE)
> > {
	static void
	exec(App & app)
	{
		say("FLIGHT_* on_mode_timeout");
		on_mode_timeout(app, ModeId::FLIGHT_NO_SIGNAL);
	}
};

template <>
struct handle<ModeId::PREFLIGHT, EventKind::SHORT_KEYPRESS, BTN_MASK_PLAY>
{
	static void
	exec(App & app)
	{
		say("PREFLIGHT SHORT_KEYPRESS PLAY");
		if (app.drone_status.ready_to_arm())
		{
			// notify drone
			app.set_mode_transition(ModeId::CONFIRM_ARM);
			say("ARM waiting confirm.");
		}
		else
		{
			app.tone.arm_failed();
			say("ARM request failed: drone is not ready to arm");
		}
	}
};

template <>
struct handle<ModeId::CONFIRM_ARM, EventKind::SHORT_KEYPRESS, BTN_MASK_TO_H>
{
	static void
	exec(App & app)
	{
		say("CONFIRM_ARM SHORT_KEYPRESS OK");
		if (app.drone_status.ready_to_arm())
		{
			app.drone_cmd.send_arm_command(app.drone_status);
		}
		else
		{
			app.tone.arm_failed();
			say("ARM confirm failed: drone is not ready to arm");
		}
	}
};

template <EventKind EVENT, ButtonId AnyButton>
struct handle< ModeId::CONFIRM_ARM, EVENT, AnyButton, When<
	EVENT == EventKind::KEY_RELEASE or EVENT == EventKind::KEY_TIMEOUT
> > {
//: Default // LONG_KEYPRESS/REPEAT_KEYPRESS does not matter
	static void
	exec(App & app)
	{
		say("CONFIRM_ARM KEY_RELEASE or KEY_TIMEOUT *");
		say("Switching back to PREFLIGHT mode.");
		app.set_mode_transition(ModeId::PREFLIGHT);
	}
};

template <ModeId MODE>
struct handle< MODE, EventKind::SHORT_KEYPRESS, BTN_MASK_POWER, When<
	MODE != ModeId::CONFIRM_ARM
> > {
	static void
	exec(App&)
	{
		say("open menu");
		// app.set_mode_transition(ModeId::MENU);
	}
};

template <>
struct handle<ModeId::FLIGHT, EventKind::SHORT_KEYPRESS, BTN_MASK_MODE>
{
	static void
	exec(App & app)
	{
		say("FLIGHT SHORT_KEYPRESS MODE");
		app.set_mode_transition(ModeId::FLIGHT_ALT);
	}
};

template <>
struct handle<ModeId::FLIGHT_ALT, EventKind::SHORT_KEYPRESS, BTN_MASK_MODE>
{
	static void
	exec(App & app)
	{
		say("FLIGHT_ALT SHORT_KEYPRESS MODE");
		app.set_mode_transition(ModeId::FLIGHT_CAM);
	}
};

template <>
struct handle<ModeId::FLIGHT_CAM, EventKind::SHORT_KEYPRESS, BTN_MASK_MODE>
{
	static void
	exec(App & app)
	{
		say("FLIGHT_CAM SHORT_KEYPRESS MODE");
		app.set_mode_transition(ModeId::FLIGHT);
	}
};

template <ModeId MODE>
struct handle< MODE, EventKind::KEY_TIMEOUT, BTN_NONE, When<
	MODE == ModeId::FLIGHT_ALT or MODE == ModeId::FLIGHT_CAM
> > {
	static void
	exec(App & app)
	{
		say("FLIGHT_ALT/FLIGHT_CAM KEY_TIMEOUT");
		app.set_mode_transition(ModeId::FLIGHT);
	}
};

//template <>
//struct handle<ModeId::FLIGHT_ALT, EventKind::KEY_TIMEOUT, BTN_NONE>
//{
//	static void
//	exec(App & app)
//	{
//		say("FLIGHT_ALT KEY_TIMEOUT");
//		app.set_mode_transition(ModeId::FLIGHT);
//	}
//};

template <ModeId MODE>
struct handle< MODE, EventKind::LONG_KEYPRESS, BTN_MASK_MODE, When<
	MODE == ModeId::FLIGHT or MODE == ModeId::FLIGHT_ALT
> > {
	static void
	exec(App & app)
	{
		say("FLIGHT LONG_KEYPRESS MODE");
		app.set_mode_transition(ModeId::SHORTCUT);
	}
};

template <EventKind EVENT, ButtonId AnyButton>
struct handle< ModeId::SHORTCUT, EVENT, AnyButton, When<
	EVENT == EventKind::KEY_RELEASE or EVENT == EventKind::KEY_TIMEOUT
> > {
	static void
	exec(App & app)
	{
		say("SHORTCUT KEY_TIMEOUT or KEY_RELEASE");
		app.set_mode_transition(ModeId::FLIGHT);
	}
};

} // end of namespace kbd_handler