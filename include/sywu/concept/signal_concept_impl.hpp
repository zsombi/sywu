#ifndef SYWU_SIGNAL_CONCEPT_IMPL_HPP
#define SYWU_SIGNAL_CONCEPT_IMPL_HPP

#include <sywu/concept/signal.hpp>
#include <sywu/concept/concept_impl.hpp>
#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/exception.hpp>
#include <sywu/wrap/functional.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/vector.hpp>

namespace sywu
{

namespace
{

template <typename FunctionType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionSlot final : public SlotImpl<ReturnType, Arguments...>
{
    using Base = SlotImpl<ReturnType, Arguments...>;

    ReturnType activateOverride(Arguments&&... args) override
    {
        return invoke(m_function, forward<Arguments>(args)...);
    }

public:
    explicit FunctionSlot(const FunctionType& function)
        : m_function(function)
    {
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodSlot final : public SlotImpl<ReturnType, Arguments...>
{
    using Base = SlotImpl<ReturnType, Arguments...>;

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        auto slotHost = m_target.lock();
        if (!slotHost)
        {
            throw bad_slot();
        }

        return invoke(m_function, slotHost, forward<Arguments>(arguments)...);
    }

public:
    using FunctionType = ReturnType(TargetObject::*)(Arguments...);
    explicit MethodSlot(shared_ptr<TargetObject> target, const FunctionType& function)
        : m_target(target)
        , m_function(function)
    {
    }

private:
    weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalSlot final : public SlotImpl<ReturnType, Arguments...>
{
    using Base = SlotImpl<ReturnType, Arguments...>;

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        if constexpr (is_void_v<ReturnType>)
        {
            invoke(*m_receiver, forward<Arguments>(arguments)...);
        }
        else
        {
            return invoke(*m_receiver, forward<Arguments>(arguments)...);
        }
    }

public:
    explicit SignalSlot(ReceiverSignal& receiver)
        : m_receiver(&receiver)
    {
    }

private:
    ReceiverSignal* m_receiver = nullptr;
};

struct ConnectionSwapper
{
    Connection previousConnection;
    explicit ConnectionSwapper(SlotPtr slot)
        : previousConnection(ActiveConnection::connection)
    {
        ActiveConnection::connection = Connection(slot);
    }
    ~ConnectionSwapper()
    {
        ActiveConnection::connection = previousConnection;
    }
};

} // namespace noname


template <class LockType, typename ReturnType, typename... Arguments>
SignalConcept<LockType, ReturnType, Arguments...>::~SignalConcept()
{
    lock_guard lock(*this);

    while (!m_slots.empty())
    {
        auto slot = m_slots.back();
        m_slots.pop_back();
        slot->disconnect();
    }
}

template <class LockType, typename ReturnType, typename... Arguments>
size_t SignalConcept<LockType, ReturnType, Arguments...>::operator()(Arguments... arguments)
{
    if (isBlocked() || m_emitGuard.isLocked())
    {
        return 0u;
    }

    lock_guard guard(m_emitGuard);

    SlotContainer slots;
    {
        lock_guard lock(*this);
        // Remove disconnected slots.
        erase_if(m_slots, [](auto& slot) { return !slot || !slot->isConnected(); });
        // Copyt the slots now.
        slots = m_slots;
    }

    auto count = int(0);
    for (auto& slot : slots)
    {
        lock_guard lock(*slot);

        try
        {
            if (!slot->isConnected())
            {
                // The slot is already disconnected from the signal, most likely due to this signal deletion.
                continue;
            }
            ConnectionSwapper backupConnection(slot);
            relock_guard relock(*slot);
            static_pointer_cast<SlotType>(slot)->activate(forward<Arguments>(arguments)...);
            ++count;
        }
        catch (const bad_weak_ptr&)
        {
            relock_guard relock(*slot);
            disconnect(Connection(slot));
        }
        catch (const bad_slot&)
        {
            relock_guard relock(*slot);
            disconnect(Connection(slot));
        }
    }

    return count;
}

template <class LockType, typename ReturnType, typename... Arguments>
Connection SignalConcept<LockType, ReturnType, Arguments...>::addSlot(SlotPtr slot)
{
    auto slotActivator = dynamic_pointer_cast<SlotType>(slot);
    SYWU_ASSERT(slotActivator);
    lock_guard lock(*this);
    m_slots.push_back(slotActivator);
    return Connection(m_slots.back());
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
SignalConcept<LockType, ReturnType, Arguments...>::connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method)
{
    using Object = typename function_traits<FunctionType>::object;
    using SlotReturnType = typename function_traits<FunctionType>::return_type;

    static_assert(
        function_traits<FunctionType>::template is_same_args<Arguments...> &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<Slot, MethodSlot<Object, SlotReturnType, Arguments...>>(receiver, method);
    slot->bind(receiver);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<!is_base_of_v<SignalConcept<LockType, ReturnType, Arguments...>, FunctionType>, Connection>
SignalConcept<LockType, ReturnType, Arguments...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename function_traits<FunctionType>::return_type;
    static_assert(
        function_traits<FunctionType>::template is_same_args<Arguments...> &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<Slot, FunctionSlot<FunctionType, SlotReturnType, Arguments...>>(function);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class RDerivedClass, typename RReturnType, class... RArguments>
Connection SignalConcept<LockType, ReturnType, Arguments...>::connect(SignalConcept<RDerivedClass, RReturnType, RArguments...>& receiver)
{
    using ReceiverSignal = SignalConcept<RDerivedClass, RReturnType, RArguments...>;
    static_assert(
        is_same_v<tuple<Arguments...>, tuple<RArguments...>>,
        "incompatible signal signature");

    auto slot = make_shared<Slot, SignalSlot<ReceiverSignal, RReturnType, Arguments...>>(receiver);
    receiver.track(slot);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
void SignalConcept<LockType, ReturnType, Arguments...>::disconnect(Connection connection)
{
    lock_guard lock(*this);
    auto slot = connection.get();
    connection.disconnect();
    erase(m_slots, slot);
}

} // namespace sywu

#endif // SYWU_SIGNAL_CONCEPT_IMPL_HPP
