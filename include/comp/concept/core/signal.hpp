#ifndef COMP_SIGNAL_CORE_HPP
#define COMP_SIGNAL_CORE_HPP

#include <comp/config.hpp>
#include <comp/utility/lockable.hpp>
#include <comp/utility/tracker.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/vector.hpp>

namespace comp { namespace core {

class Signal;

/// Core of the slots.
template <typename LockType>
class COMP_API Slot : public Lockable<LockType>, public enable_shared_from_this<Slot<LockType>>
{
public:
    /// ConnectionTracker interface.
    struct COMP_API TrackerInterface
    {
        /// Destructor.
        virtual ~TrackerInterface() = default;

        /// Detaches a slot from a tracker.
        virtual void untrack() = 0;

        /// Returns the valid state of a tracker. A tracker is valid when it tracks a valid object.
        /// \return If the tracker is valid, returns \e true, otherwise \e false.
        virtual bool isValid() const = 0;
    };
    using TrackerPtr = shared_ptr<TrackerInterface>;

    virtual ~Slot() = default;

    /// Checks whether a slot is connected.
    /// \return If the slot is connected, returns \e true, otherwise returns \e false.
    bool isConnected() const;

    /// Disconnects a slot.
    void disconnect();

    /// Adds a tracker to the slot.
    /// \param tracker The tracker to add to the slot.
    /// \see Connection::bind()
    void addTracker(TrackerPtr tracker);

protected:
    /// Constructor.
    explicit Slot(Signal& signal)
        : m_signal(&signal)
    {
    }

    /// To implement slot specific disconnect function, override this method.
    virtual void disconnectOverride()
    {
    }

    /// The container with the binded trackers.
    using TrackersContainer = vector<TrackerPtr>;
    TrackersContainer m_trackers;

    /// The signal to which the slot connects.
    Signal* m_signal = nullptr;

    /// The connected state.
    atomic_bool m_isConnected = true;
};

/// Core of the signals.
class COMP_API Signal
{
public:
    using SlotCore = Slot<mutex>;

    /// The Connection holds a slot connected to a signal. It is a token to a receiver slot connected to
    /// that signal.
    class COMP_API Connection
    {
    public:
        using SlotPtr = shared_ptr<SlotCore>;
        using SlotWeakPtr = weak_ptr<SlotCore>;

        /// Constructor.
        Connection() = default;

        /// Constructs the connection with a \a slot.
        Connection(SlotPtr slot)
            : m_slot(slot)
        {
        }

        /// Destructor.
        ~Connection() = default;

        /// Disconnects the slot.
        void disconnect()
        {
            auto slot = m_slot.lock();
            if (!slot)
            {
                return;
            }
            slot->disconnect();
        }

        /// Returns the valid state of the connection.
        /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
        /// source signal or its trackers are destroyed.
        operator bool() const
        {
            const auto slot = m_slot.lock();
            return slot && slot->isConnected();
        }

        /// Returns the slot of the connection.
        /// \return The slot of the connection. If the connection is not valid, returns \e nullptr.
        SlotPtr get() const
        {
            return m_slot.lock();
        }

    protected:
        SlotWeakPtr m_slot;
    };


    /// Destructor.
    virtual ~Signal() = default;

    /// Disconnects a \a connection.
    /// \param connection The connection to disconnect.
    virtual void disconnect(Connection connection) = 0;
};

}} // comp::core

#endif // COMP_SIGNAL_CORE_HPP
