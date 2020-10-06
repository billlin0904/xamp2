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
		  PLAYER_STATE_INIT,
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
			XAMP_LOG_DEBUG("Running {} state.", EnumToString(statePtr->GetState()));
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
		XAMP_LOG_DEBUG("Ignore state.");
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
	constexpr PlayerState GetState() const noexcept {
		return PlayerState::PLAYER_STATE_INIT;
	}

	TransitionTo<PlayerRunning> Handle(const PlayingEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_RUNNING.");
		return {};
	}

	TransitionTo<PlayerStoped> Handle(const StopEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_STOPPED.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}

	Ignore Handle(const ResumeEvent&) const {
		return {};
	}

	Ignore Handle(const PauseEvent&) const {
		return {};
	}
};

struct PlayerRunning {
	constexpr PlayerState GetState() const noexcept {
		return PlayerState::PLAYER_STATE_RUNNING;
	}

	TransitionTo<PlayerStoped> Handle(const StopEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_STOPPED.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}

	Ignore Handle(const PlayingEvent&) const {
		return {};
	}

	Ignore Handle(const ResumeEvent&) const {
		return {};
	}

	TransitionTo<PlayerPaused> Handle(const PauseEvent&) const {
		return {};
	}
};


struct PlayerPaused {
	constexpr PlayerState GetState() const noexcept {
		return PlayerState::PLAYER_STATE_PAUSED;
	}

	TransitionTo<PlayerPaused> Handle(const PlayingEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_PAUSED.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}

	Ignore Handle(const PauseEvent&) const {
		return {};
	}

	Ignore Handle(const ResumeEvent&) const {
		return {};
	}

	Ignore Handle(const StopEvent&) const {
		return {};
	}	
};

struct PlayerRusume {
	constexpr PlayerState GetState() const noexcept {
		return PlayerState::PLAYER_STATE_RESUME;
	}

	TransitionTo<PlayerPaused> Handle(const PauseEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_PAUSED.");
		return {};
	}

	Ignore Handle(const InitEvent&) const {
		return {};
	}

	TransitionTo<PlayerRunning> Handle(const ResumeEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_RUNNING.");
		return {};
	}

	Ignore Handle(const PlayingEvent&) const {
		return {};
	}

	Ignore Handle(const StopEvent&) const {
		return {};
	}
};

struct PlayerStoped {
	constexpr PlayerState GetState() const noexcept {
		return PlayerState::PLAYER_STATE_STOPPED;
	}

	TransitionTo<PlayerInit> Handle(const InitEvent&) const {
		XAMP_LOG_DEBUG("Change to PLAYER_STATE_INIT.");
		return {};
	}

	Ignore Handle(const PlayingEvent&) const {		
		return {};
	}

	Ignore Handle(const StopEvent&) const {
		return {};
	}

	Ignore Handle(const PauseEvent&) const {
		return {};
	}

	Ignore Handle(const ResumeEvent&) const {
		return {};
	}
};

using PlayerStateMachine = StateMachine<PlayerStoped, PlayerInit, PlayerRunning, PlayerPaused, PlayerRusume>;

}
