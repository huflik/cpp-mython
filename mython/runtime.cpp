#include "runtime.h"

using namespace std;

namespace runtime {

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder& object) {
        if (auto obj = object.TryAs<Bool>()) {
            return obj->GetValue() == true;
        }
        if (auto obj = object.TryAs<Number>()) {
            return !(obj->GetValue() == 0);
        }
        if (auto obj = object.TryAs<String>()) {
            return !(obj->GetValue().empty());
        }
        return false;
    }

    void ClassInstance::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        if (HasMethod(detail::STR_METHOD, 0)) {
            Call(detail::STR_METHOD, {}, context).Get()->Print(os, context);
        }
        else {
            os << this;
        }
    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        if (auto method_ptr = cls_.GetMethod(method)) {
            if (method_ptr->formal_params.size() == argument_count) {
                return true;
            }
        }
        return false;
    }

    Closure& ClassInstance::Fields() {
        return closure_;
    }

    const Closure& ClassInstance::Fields() const {
        return closure_;
    }

    ClassInstance::ClassInstance(const Class& cls) : cls_(cls) {
    }

    ObjectHolder ClassInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context) {
        if (HasMethod(method, actual_args.size())) {
            auto method_ptr = cls_.GetMethod(method);
            Closure map_args;
            map_args["self"] = ObjectHolder::Share(*this);
            size_t index = 0;
            for (auto& param : method_ptr->formal_params) {
                map_args[param] = actual_args[index++];
            }
            return method_ptr->body->Execute(map_args, context);
        }
        else {
            throw std::runtime_error("No method"s);
        }
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent) : name_(std::move(name)), methods_(std::move(methods)), parent_(parent) {
    }

    const Method* Class::GetMethod(const std::string& name) const {
        const auto it = find_if(methods_.begin(), methods_.end(), [&name](const Method& method) {
            return method.name == name;
            });
        if (it != methods_.end()) {
            return &(*it);
        }
        if (parent_) {
            return parent_->GetMethod(name);
        }
        return nullptr;
    }

    const string& Class::GetName() const {
        return name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
        os << "Class "s << GetName();
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }


    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (auto ptr = lhs.TryAs<ClassInstance>()) {
            return IsTrue(ptr->Call(detail::EQ_METHOD, { rhs }, context));
        }
        if (auto res = detail::Compare(lhs, rhs, equal_to{})) {
            return res.value();
        }
        if (!bool(lhs) && !bool(rhs)) {
            return true;
        }
        throw std::runtime_error("Error Equal compare"s);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (auto ptr = lhs.TryAs<ClassInstance>()) {
            return IsTrue(ptr->Call(detail::LT_METHOD, { rhs }, context));
        }
        if (auto res = detail::Compare(lhs, rhs, less{})) {
            return res.value();
        }
        throw std::runtime_error("Error Less compare"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !LessOrEqual(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
    }

}  // namespace runtime