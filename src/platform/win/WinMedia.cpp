// Windows implementation: "now playing" through the system media transport controls
// (the same source as the media flyout next to the volume OSD: Spotify, browsers, players...).
//
// Uses C++/WinRT (ships with the Windows SDK, MSVC only). All WinRT calls run on one worker
// thread in the multithreaded apartment: blocking .get() is not allowed on the UI (STA) thread.

#include "platform/Platform.h"

#include <windows.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace platform {

  namespace {

    namespace wmc = winrt::Windows::Media::Control;
    namespace wss = winrt::Windows::Storage::Streams;

    std::int64_t nowMs() {
      using namespace std::chrono;
      return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    class WinMediaController final : public MediaController {
    public:
      explicit WinMediaController(std::function<void(const MediaState&)> onChange) : m_onChange(std::move(onChange)) {
        m_thread = std::thread([this] { run(); });
      }

      ~WinMediaController() override {
        {
          std::lock_guard lock(m_mutex);
          m_quit = true;
        }
        m_cv.notify_all();
        if (m_thread.joinable()) {
          m_thread.join();
        }
      }

      void playPause() override { post(Command::PlayPause); }
      void next() override { post(Command::Next); }
      void previous() override { post(Command::Previous); }
      void seek(double seconds) override {
        m_seekTarget = seconds;
        post(Command::Seek);
      }

    private:
      enum class Command { PlayPause, Next, Previous, Seek };

      void post(Command c) {
        {
          std::lock_guard lock(m_mutex);
          m_commands.push_back(c);
        }
        m_cv.notify_all();
      }

      void markDirty() {
        m_dirty = true;
        m_cv.notify_all();
      }

      void attach(const wmc::GlobalSystemMediaTransportControlsSession& session) {
        m_propsToken = {};
        m_playbackToken = {};
        m_timelineToken = {};
        m_session = session;
        if (!m_session) {
          return;
        }
        m_propsToken = m_session.MediaPropertiesChanged(winrt::auto_revoke, [this](auto&&, auto&&) { markDirty(); });
        m_playbackToken = m_session.PlaybackInfoChanged(winrt::auto_revoke, [this](auto&&, auto&&) { markDirty(); });
        m_timelineToken = m_session.TimelinePropertiesChanged(winrt::auto_revoke, [this](auto&&, auto&&) { markDirty(); });
      }

      void run() {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        try {
          m_manager = wmc::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        } catch (...) {
          m_manager = nullptr;
        }
        if (m_manager) {
          m_sessionToken = m_manager.CurrentSessionChanged(winrt::auto_revoke, [this](auto&&, auto&&) {
            m_sessionChanged = true;
            markDirty();
          });
          attach(m_manager.GetCurrentSession());
        }
        publish();

        while (true) {
          std::deque<Command> commands;
          {
            std::unique_lock lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(1000),
                          [this] { return m_quit || !m_commands.empty() || m_dirty.load(); });
            if (m_quit) {
              break;
            }
            commands.swap(m_commands);
          }
          for (const Command c : commands) {
            runCommand(c);
          }
          if (m_sessionChanged.exchange(false) && m_manager) {
            try {
              attach(m_manager.GetCurrentSession());
            } catch (...) {
            }
          }
          m_dirty = false;
          publish();
        }

        m_propsToken = {};
        m_playbackToken = {};
        m_timelineToken = {};
        m_sessionToken = {};
        m_session = nullptr;
        m_manager = nullptr;
      }

      void runCommand(Command c) {
        if (!m_session) {
          return;
        }
        try {
          switch (c) {
          case Command::PlayPause:
            m_session.TryTogglePlayPauseAsync().get();
            break;
          case Command::Next:
            m_session.TrySkipNextAsync().get();
            break;
          case Command::Previous:
            m_session.TrySkipPreviousAsync().get();
            break;
          case Command::Seek: {
            const auto ticks = static_cast<std::int64_t>(m_seekTarget.load() * 10'000'000.0);
            m_session.TryChangePlaybackPositionAsync(ticks).get();
            break;
          }
          }
        } catch (...) {
        }
      }

      void publish() {
        MediaState st;
        try {
          if (m_session) {
            st.available = true;
            st.appId = m_session.SourceAppUserModelId().c_str();

            const auto props = m_session.TryGetMediaPropertiesAsync().get();
            if (props) {
              st.title = props.Title().c_str();
              st.artist = props.Artist().c_str();
              st.album = props.AlbumTitle().c_str();

              const std::wstring trackKey = st.title + L"\x1f" + st.artist + L"\x1f" + st.album;
              if (trackKey != m_lastTrackKey) {
                m_lastTrackKey = trackKey;
                m_art.clear();
                if (const auto thumb = props.Thumbnail()) {
                  try {
                    const auto stream = thumb.OpenReadAsync().get();
                    const auto size = static_cast<std::uint32_t>(stream.Size());
                    if (size > 0 && size < 16 * 1024 * 1024) {
                      wss::DataReader reader(stream);
                      reader.LoadAsync(size).get();
                      m_art.resize(size);
                      reader.ReadBytes(m_art);
                    }
                  } catch (...) {
                    m_art.clear();
                  }
                }
                ++m_artVersion;
              }
            }

            const auto playback = m_session.GetPlaybackInfo();
            if (playback) {
              st.playing = playback.PlaybackStatus() == wmc::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
              const auto controls = playback.Controls();
              st.canNext = controls.IsNextEnabled();
              st.canPrevious = controls.IsPreviousEnabled();
              st.canSeek = controls.IsPlaybackPositionEnabled();
            }

            const auto timeline = m_session.GetTimelineProperties();
            if (timeline) {
              const double start = static_cast<double>(timeline.StartTime().count()) / 1e7;
              const double end = static_cast<double>(timeline.EndTime().count()) / 1e7;
              double position = static_cast<double>(timeline.Position().count()) / 1e7 - start;
              // Players only report the position now and then: extrapolate while playing.
              const auto updated = timeline.LastUpdatedTime();
              const auto now = winrt::clock::now();
              if (st.playing && updated.time_since_epoch().count() > 0 && now > updated) {
                position += std::chrono::duration<double>(now - updated).count();
              }
              st.duration = end > start ? end - start : 0;
              st.position = st.duration > 0 ? std::min(std::max(position, 0.0), st.duration) : std::max(position, 0.0);
              st.positionTimestampMs = nowMs();
            }
          }
        } catch (...) {
          st = MediaState{};
          m_session = nullptr;
        }
        st.artVersion = m_artVersion;
        st.art = m_art;
        if (m_onChange) {
          m_onChange(st);
        }
      }

      std::function<void(const MediaState&)> m_onChange;
      std::thread m_thread;
      std::mutex m_mutex;
      std::condition_variable m_cv;
      std::deque<Command> m_commands;
      bool m_quit = false;
      std::atomic<bool> m_dirty{false};
      std::atomic<bool> m_sessionChanged{false};
      std::atomic<double> m_seekTarget{0};

      wmc::GlobalSystemMediaTransportControlsSessionManager m_manager{nullptr};
      wmc::GlobalSystemMediaTransportControlsSession m_session{nullptr};
      wmc::GlobalSystemMediaTransportControlsSessionManager::CurrentSessionChanged_revoker m_sessionToken;
      wmc::GlobalSystemMediaTransportControlsSession::MediaPropertiesChanged_revoker m_propsToken;
      wmc::GlobalSystemMediaTransportControlsSession::PlaybackInfoChanged_revoker m_playbackToken;
      wmc::GlobalSystemMediaTransportControlsSession::TimelinePropertiesChanged_revoker m_timelineToken;

      std::wstring m_lastTrackKey;
      std::vector<std::uint8_t> m_art;
      std::uint64_t m_artVersion = 0;
    };

  } // namespace

  std::unique_ptr<MediaController> MediaController::create(std::function<void(const MediaState&)> onChange) {
    return std::make_unique<WinMediaController>(std::move(onChange));
  }

} // namespace platform
