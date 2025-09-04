#pragma once

#include "nois/NoisMacros.hpp"

#define NOIS_INTERFACE_GETTER_IMPL(_class, _name, _type) \
	Ref_t<_type> _class::Get##_name() const { return m_Impl->Get##_name(); }

#define NOIS_INTERFACE_SETTER_IMPL(_class, _name, _type) \
	void _class::Set##_name(Ref_t<_type> value) { m_Impl->Set##_name(value); }

#define NOIS_INTERFACE_PARAM_IMPL(_class, _name, _type) \
	NOIS_INTERFACE_GETTER_IMPL(_class, _name, _type) \
	NOIS_INTERFACE_SETTER_IMPL(_class, _name, _type)

#define NOIS_IMPL_PARAM(_name, _type) \
	Ref_t<_type> m_##_name; \
	Ref_t<_type> Get##_name() const { return m_##_name; } \
	void Set##_name(Ref_t<_type> value) { m_##_name = value; }
