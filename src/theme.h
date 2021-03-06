#pragma once

#include <string>
#include <vector>

#include "attribute_.h"
#include "object.h"

/** The proxy interface
 */

class ProxyAddTargetInterface {
public:
    /* Propagate all attribute writes to an attribute of the same name of the
     * given object
     */
    virtual void addProxyTarget(Object* object) = 0;
    /** the following is just a hack such that we need to write the list of
     * attributes only once in the constructor of DecorationScheme
     */
    virtual Attribute* toAttribute() = 0;
};

/** An attribute that is at the same time a proxy
 * to attributes with the same name in other objects.
 */
template<typename T>
class AttributeProxy_ : public Attribute_<T>, public ProxyAddTargetInterface {
public:
    AttributeProxy_(const std::string &name, const T &payload)
        : Attribute_<T>(name, payload)
    {
    }

    std::string change(const std::string &payload_str) override {
        std::string msg = Attribute_<T>::change(payload_str);
        if (msg.empty()) {
            // propagate the new attribute value
            // ignoring their error message since we assume that
            // they have the same type / validation as this attribute
            for (auto target : targetObjects_) {
                Attribute* a = target->attribute(this->name());
                if (a != nullptr) {
                    a->change(payload_str);
                }
            }
        }
        return msg;
    }
    void addProxyTarget(Object* object) override {
        targetObjects_.push_back(object);
    }
    bool resetValue() override {
        bool res = Attribute_<T>::resetValue();
        if (res) {
            for (auto target : targetObjects_) {
                Attribute* a = target->attribute(this->name());
                if (a != nullptr) {
                    a->resetValue();
                }
            }
        }
        return res;
    }
    Attribute* toAttribute() override {
        return this;
    }
private:
    std::vector<Object*> targetObjects_;
};

class DecorationScheme : public Object {
public:
    DecorationScheme();
    ~DecorationScheme() override = default;
    DynAttribute_<std::string> reset;
    AttributeProxy_<unsigned long>     border_width = {"border_width", 0};
    AttributeProxy_<Color>   border_color = {"color", {"black"}};
    AttributeProxy_<bool>    tight_decoration = {"tight_decoration", false}; // if set, there is no space between the
                              // decoration and the window content
    AttributeProxy_<Color>   inner_color = {"inner_color", {"black"}};
    AttributeProxy_<unsigned long>     inner_width = {"inner_width", 0};
    AttributeProxy_<Color>   outer_color = {"outer_color", {"black"}};
    AttributeProxy_<unsigned long>     outer_width = {"outer_width", 0};
    AttributeProxy_<unsigned long>     padding_top = {"padding_top", 0};    // additional window border
    AttributeProxy_<unsigned long>     padding_right = {"padding_right", 0};  // additional window border
    AttributeProxy_<unsigned long>     padding_bottom = {"padding_bottom", 0}; // additional window border
    AttributeProxy_<unsigned long>     padding_left = {"padding_left", 0};   // additional window border
    AttributeProxy_<Color>   background_color = {"background_color", {"black"}}; // color behind client contents

    Signal scheme_changed_; //! whenever one of the attributes changes.

    Rectangle inner_rect_to_outline(Rectangle rect) const;
    Rectangle outline_to_inner_rect(Rectangle rect) const;

    // after having called this with some vector 'decs', then if an attribute
    // is changed here, then the attribute with the same name is changed
    // accordingly in each of the elements of 'decs'.
    void makeProxyFor(std::vector<DecorationScheme*> decs);
private:
    std::string resetSetterHelper(std::string dummy);
    std::string resetGetterHelper();
    std::vector<ProxyAddTargetInterface*> proxyAttributes_;
};

class DecTriple : public DecorationScheme {
public:
    DecTriple();
    DecorationScheme  normal;
    DecorationScheme  active;
    DecorationScheme  urgent;
    //! whenever one of the normal, active, urgend changed
    //! (but not when the proxy attributes are changed)
    Signal triple_changed_;
    // pick the right scheme, depending on whether a window is active/urgent
    const DecorationScheme& operator()(bool if_active, bool if_urgent) const {
        if (if_active) {
            return this->active;
        } else if (if_urgent) {
            return this->urgent;
        } else {
            return normal;
        }
    }
};

class Theme : public DecTriple {
public:
    enum class Type {
        Fullscreen,
        Tiling,
        Floating,
        Minimal,
        Count,
    };
    const DecTriple& operator[](Type t) const {
        return dec[(int)t];
    };
    Theme();

    Signal theme_changed_; //! one of the attributes in one of the triples changed

    // a sub-decoration for each type
    DecTriple dec[(int)Type::Count];
};


