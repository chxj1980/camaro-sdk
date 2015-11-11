#include "ExtensionAccess.h"
#include "System.h"

using namespace TopGear;
using namespace Win;

ExtensionAccess::ExtensionAccess(std::shared_ptr<IMExtensionLite> &validator) 
	: extensionAgent(validator)
{
	pXu = extensionAgent->GetExtensionUnit();
}

std::unique_ptr<uint8_t[]> ExtensionAccess::GetProperty(int index, int& len)
{
	len = extensionAgent->GetLen(index);
	std::unique_ptr<uint8_t[]> data(new uint8_t[len]{ 0 });
	auto res = pXu->get_Property(index, len, data.get());
	if (res != S_OK)
		return std::unique_ptr<uint8_t[]>();
	return data;
}

int ExtensionAccess::SetProperty(int index, const uint8_t* data, size_t size)
{
	auto len = extensionAgent->GetLen(index);
	if (len < size)
		return -1;
	if (len == size)
		return pXu->put_Property(index, len, const_cast<uint8_t *>(data));

	std::unique_ptr<uint8_t[]> buffer(new uint8_t[len]{ 0 });
	memcpy(buffer.get(), data, size);
	return pXu->put_Property(index, len, buffer.get());
}

ExtensionAccess::~ExtensionAccess()
{
	//System::SafeRelease(&pXu);
}
