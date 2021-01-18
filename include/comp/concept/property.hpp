#ifndef COMP_PROPERTY_CONCEPT_HPP
#define COMP_PROPERTY_CONCEPT_HPP

#include <comp/config.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/concept/zero_safe_container.hpp>
#include <comp/signal.hpp>

namespace comp
{

/// The enum defines the status of a property value.
enum class PropertyValueStatus
{
    /// The property value is detached.
    Detached,
    /// The property value is attaching.
    Attaching,
    /// The property value is detaching.
    Detaching,
    /// The property value is attached, and it is the active property value.
    Active,
    /// The property value is attached, active and evaluating.
    Evaluating,
    /// The property value is attached and inactive.
    Inactive
};

/// The enum defines the property value write behavior.
enum class WriteBehavior
{
    /// The property value is kept when the property setter is called.
    Keep,
    /// The property value is discarded when the property setter is called.
    Discard
};

template <typename T>
class PropertyCore;

/// The PropertyValue template defines the interface of property values. Property values are interfaces
/// that provide the actual value of a property. A property may have several property value providers, of
/// which only one is active.
template <typename T>
class COMP_TEMPLATE_API PropertyValue : public Lockable<mutex>, public enable_shared_from_this<PropertyValue<T>>
{
public:
    using DataType = T;

    /// The behavior of the property value when the property setter is invoked.
    const WriteBehavior writeBehavior = WriteBehavior::Discard;

    /// Destructor.
    virtual ~PropertyValue() = default;

    /// Returns the status of the property value.
    /// \return The status of the property value.
    PropertyValueStatus getStatus() const
    {
        return m_status;
    }

    /// Evaluates the property value, and returns the evaluated value, which represents the value of the property.
    /// \return The value of the property.
    DataType evaluate();

    /// Stores a value in the property value.
    /// \param value The value to store in the property value.
    /// \return If the property value is changed, returns \e true, otherwise \e false.
    void set(const DataType& value);

    /// Swaps the data of two property values.
    void swap(PropertyValue& other);

    /// Checks if the property value is the active property value.
    bool isActive() const;

    /// Activates a property value. The property value must be attached to a property.
    void activate();

    /// Deactivates a property value. The property value must be attached and active.
    void deactivate();

    /// Attaches the property value to a \a property. The property value must not be attached to any property.
    void attach(PropertyCore<T>& property);

    /// Detaches the property value from a property.
    void detach();

protected:
    /// Constructor.
    explicit PropertyValue(WriteBehavior writeBehavior)
        : writeBehavior(writeBehavior)
    {
    }

    /// \name Overridables
    /// \{

    /// Ovberridable to evaluate a property value. Returns the result of the evaluation.
    /// \return The result of the evaluation.
    virtual DataType evaluateOverride() = 0;

    /// Value setter overridable.
    virtual bool setOverride(const DataType&) = 0;

    /// Value swapper overridable.
    virtual void swapOverride(PropertyValue&) = 0;
    /// \}

    /// The target property to which the property value is attached.
    PropertyCore<T>* m_target = nullptr;
    /// The status of a property value.
    PropertyValueStatus m_status = PropertyValueStatus::Detached;
};

template <typename T>
using PropertyValuePtr = shared_ptr<PropertyValue<T>>;
template <typename T>
using PropertyValueWeakPtr = weak_ptr<PropertyValue<T>>;

template <typename T>
class COMP_TEMPLATE_API PropertyCore
{
public:
    using ValuePtr = PropertyValuePtr<T>;
    using ChangedSignal = Signal<void(T const&)>;

    /// The changed signal emits when the value of the property is changed. The signal is also
    /// emitted when the active property value is changed.
    ChangedSignal changed;

protected:
    explicit PropertyCore() = default;
};

/// The StateConcept template defines the core functionality of state properties. State properties are read-only
/// properties. State properties have a single property value provider. To set the value of the property, use the
/// set method of the property value provider you pass to the constructor.
template <typename T>
class COMP_TEMPLATE_API StateConcept : public PropertyCore<T>
{
    using Base = PropertyCore<T>;

protected:
    /// Constructor.
    explicit StateConcept(typename Base::ValuePtr propertyValue);

    /// The property value provider.
    typename Base::ValuePtr m_value;
};

/// The PropertyConcept template defines the core functionality of the properties. Provides property value
/// provider management, as well as notification of the property value changes.
/// A property must have at least one property value provider with WriteBehavior::Keep.
template <typename T>
class COMP_TEMPLATE_API PropertyConcept : public PropertyCore<T>
{
    using Base = PropertyCore<T>;

public:
    /// Adds a property value to a property. The property value becomes the active property value.
    /// \param propertyValue The property value to add.
    void addPropertyValue(typename Base::ValuePtr propertyValue);

    /// Removes a property value from a property. If the property value is the active property value,
    /// the last added property value becomes the active property value for the property.
    /// \param propertyValue The property value to remove.
    void removePropertyValue(PropertyValue<T>& propertyValue);

protected:
    /// Constructor.
    explicit PropertyConcept(typename Base::ValuePtr defaultValue);

    /// Removes the discardable property values. Makes the last added value provider that is kept as
    /// the active value provider.
    void discard();

    /// Returns the active property value of the property.
    typename Base::ValuePtr getActiveValue() const;

    struct VPNullCheck
    {
        bool operator()(typename Base::ValuePtr propertyValue)
        {
            return !propertyValue;
        }
    };

    struct VPInvalidator
    {
        void operator()(typename Base::ValuePtr& propertyValue)
        {
            propertyValue->detach();
            propertyValue.reset();
        }
    };

    ZeroSafeContainer<typename Base::ValuePtr, VPNullCheck, VPInvalidator> m_vp;
    PropertyValueWeakPtr<T> m_active;
};

} // namespace comp

#endif // COMP_PROPERTY_CONCEPT_HPP