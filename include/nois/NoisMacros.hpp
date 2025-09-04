#pragma once

#define NOIS_INTERFACE_GETTER(_name, _type) \
	Ref_t<_type> Get##_name() const;

#define NOIS_INTERFACE_SETTER(_name, _type) \
	void Set##_name(Ref_t<_type> value);

#define NOIS_INTERFACE_PARAM(_name, _type) \
	NOIS_INTERFACE_GETTER(_name, _type) \
	NOIS_INTERFACE_SETTER(_name, _type)
