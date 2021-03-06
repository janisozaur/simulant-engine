//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "deps/kazlog/kazlog.h"
#include "window.h"
#include "stage.h"
#include "sound.h"
#include "sound_driver.h"

namespace smlt {

Sound::Sound(SoundID id, ResourceManager *resource_manager, SoundDriver *sound_driver):
    generic::Identifiable<SoundID>(id),
    Resource(resource_manager),
    driver_(sound_driver) {


}

SourceInstance::SourceInstance(Source &parent, SoundID sound, bool loop_stream):
    parent_(parent),
    source_(0),
    buffers_{0, 0},
    sound_(sound),
    loop_stream_(loop_stream),
    is_dead_(false) {


    SoundDriver* driver = parent_._sound_driver();

    source_ = driver->generate_sources(1).back();
    buffers_ = driver->generate_buffers(2);
}

SourceInstance::~SourceInstance() {
    SoundDriver* driver = parent_._sound_driver();

    driver->stop_source(source_); // Make sure we have stopped playing!
    driver->delete_sources({source_});
    driver->delete_buffers(buffers_);
}

void SourceInstance::start() {
    //Fill up two buffers to begin with
    auto bs1 = stream_func_(buffers_[0]);
    auto bs2 = stream_func_(buffers_[1]);

    int to_queue = (bs1 && bs2) ? 2 : (bs1 || bs2)? 1 : 0;

    SoundDriver* driver = parent_._sound_driver();

    driver->queue_buffers_to_source(source_, to_queue, buffers_);
    driver->play_source(source_);
}

bool SourceInstance::is_playing() const {
    SoundDriver* driver = parent_._sound_driver();
    return driver->source_state(source_) == AUDIO_SOURCE_STATE_PLAYING;
}

void SourceInstance::update(float dt) {
    SoundDriver* driver = parent_._sound_driver();

    int32_t processed = driver->source_buffers_processed_count(source_);

    while(processed--) {
        AudioBufferID buffer = driver->unqueue_buffers_from_source(source_, 1).back();

        uint32_t bytes = stream_func_(buffer);

        if(!bytes) {
            parent_.signal_stream_finished_();
            if(loop_stream_) {
                //Restart the sound
                auto sound = parent_.stage_->assets->sound(sound_);
                sound->init_source_(*this);
                start();
            } else {
                //Mark as dead
                is_dead_ = true;
            }
        } else {
            driver->queue_buffers_to_source(source_, 1, {buffer});
        }
    }
}

Source::Source(Window *window):
    stage_(nullptr),
    window_(window) {

}

Source::Source(Stage *stage, SoundDriver* driver):
    stage_(stage),
    window_(nullptr),
    driver_(driver) {


}

Source::~Source() {

}

void Source::play_sound(SoundID sound, bool loop) {
    if(!sound) {
        L_WARN("Tried to play an invalid sound");
        return;
    }

    SourceInstance::ptr new_source = SourceInstance::create(*this, sound, loop);

    /* This is surely wrong... */
    if(stage_) {
        auto s = stage_->assets->sound(sound);
        s->init_source_(*new_source);
    } else {
        auto s = window_->shared_assets->sound(sound);
        s->init_source_(*new_source);
    }

    new_source->start();

    instances_.push_back(new_source);
}

void Source::update_source(float dt) {
    for(auto instance: instances_) {
        instance->update(dt);
    }

    //Remove any instances that have finished playing
    instances_.erase(
        std::remove_if(
            instances_.begin(),
            instances_.end(),
            std::bind(&SourceInstance::is_dead, std::placeholders::_1)
        ),
        instances_.end()
    );
}

SoundDriver *Source::_sound_driver() const {
    return (window_) ? window_->_sound_driver() : driver_;
}

int32_t Source::playing_sound_count() const {
    int32_t i = 0;
    for(auto instance: instances_) {
        if(instance->is_playing()) {
            i++;
        }
    }
    return i;
}

}
