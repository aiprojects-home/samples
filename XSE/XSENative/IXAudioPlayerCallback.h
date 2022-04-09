#pragma once

#include "XException.h"

// ��������� ��� ������� �����������.
class IXAudioPlayerCallback
{
public:

	// ���������� ����� ����������� ��������� ��������������� (�� �������, � ��� ���������� �����).
	virtual void OnStreamingEnd(uint16_t id) = 0;

	// ����� ��� ���������� ������.
	virtual void OnError(const uint16_t id, const XException e) = 0;
};
