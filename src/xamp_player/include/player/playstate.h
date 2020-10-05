//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <variant>
#include <mutex>

#include <base/logger.h>
#include <base/enum.h>

namespace xamp::player {

MAKE_ENUM(PlayerState,
	      PLAYER_STATE_RUNNING,
	      PLAYER_STATE_PAUSED,
          PLAYER_STATE_RESUME,
          PLAYER_STATE_STOPPED)

template <typename... States>
class StateMachine {
public:
	template <typename State>
	void TransitionTo() {
		current_state_ = &std::get<State>(states_);
	}

	template <typename Event>
	void Handle(const Event & event) {
		std::lock_guard guard{ mutex_ };
		auto pass_event_to_state = [this, &event](auto statePtr) {
			statePtr->Handle(event).Execute(*this);
		};
		std::visit(pass_event_to_state, current_state_);		
	}

private:
	std::tuple<States...> states_;
	std::variant<States*...> current_state_{ &std::get<0>(states_) };
	std::mutex mutex_;
};

template <typename State>
struct TransitionTo {
	template <typename Machine>
	void Execute(Machine& machine) {
		machine.template TransitionTo<State>();
	}
};

struct Ignore {
	template <typename Machine>
	void Execute(Machine&) {
		XAMP_LOG_DEBUG("Invalid state.");
	}
};

struct InitEvent {};
struct PlayingEvent {};
struct PauseEvent {};
struct ResumeEvent {};
struct StopEvent {};

struct PlayerInit;
struct PlayerRunning;
struct PlayerPaused;
struct PlayerRusume;
struct PlayerStoped;

struct PlayerInit {
	TransitionTo<PlayerRunning> Handle(const PlayingEvent&) const {
		XAMP_LOG_DEBUG("Change to PlayerRunning.");
		return {};
	}

	TransitionTo<PlayerStoped> Handle(const StopEvent&) const {
		XAMP_LOG_DEBUG("Change to PlayerStoped.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}
};

struct PlayerRunning {
	TransitionTo<PlayerStoped> Handle(const StopEvent&) const {
		XAMP_LOG_DEBUG("Change to PlayerStoped.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}

	Ignore Handle(const PlayingEvent&) const {
		return {};
	}
};

//
//struct PlayerPaused {
//	TransitionTo<PlayerPaused> Handle(const PlayingEvent&) const {
//		return {};
//	}
//	Nothing Handle(const PauseEvent&) const {
//		return {};
//	}
//};
//
//struct PlayerRusume {
//	TransitionTo<PlayerRunning> Handle(const PauseEvent&) const {
//		return {};
//	}
//
//	Nothing Handle(const ResumeEvent&) const {
//		return {};
//	}
//};

struct PlayerStoped {
	TransitionTo<PlayerInit> Handle(const InitEvent&) const {
		XAMP_LOG_DEBUG("Change to PlayerInit.");
		return {};
	}

	Ignore Handle(const PlayingEvent&) const {		
		return {};
	}

	Ignore Handle(const StopEvent&) const {
		return {};
	}
};

using PlayerStateMachine = StateMachine<PlayerStoped, PlayerInit, PlayerRunning/*, PlayerPaused, PlayerRusume*/>;

}
