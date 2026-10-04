#include "unimplemented.h"
#include <string.h>
#include <limits.h>

void sync_write_header(unsigned char *data, const sync_frame_info_t *info)
{
	memcpy(data, "SYNC", 4);
	data[4] = 1;
	data[5] = (unsigned char)info->kind;
	data[6] = data[7] = 0;
	memcpy(data + 8, info->session_id.bytes, 16);
	uint64_t fields[] = { info->stream_id, info->sequence, info->tick,
		info->payload_bytes };
	for (unsigned f = 0; f < 4; f++) {
		unsigned width = f == 3 ? 4 : 8;
		for (unsigned i = 0; i < width; i++)
			data[24 + f * 8 + i] =
				(unsigned char)(fields[f] >> (8 * i));
	}
	memset(data + 52, 0, 4);
}

sync_result_t sync_frame_inspect(const void *data, size_t size,
	sync_frame_info_t *out_info)
{
	/*
	 * Zero output; validate pointers and exact complete-frame length.
	 * Decode envelope without native struct casts; check version/kind.
	 * Return envelope only, without allocation, world access or payload
	 * work.
	 */
	if (out_info)
		memset(out_info, 0, sizeof(*out_info));
	if (!data || !size || !out_info || size > UINT32_MAX)
		return SYNC_ERR_ARGUMENT;
	if (size < SYNC_HEADER)
		return SYNC_ERR_PROTOCOL;
	const unsigned char *p = data;
	if (memcmp(p, "SYNC", 4))
		return SYNC_ERR_PROTOCOL;
	if (p[4] != 1)
		return SYNC_ERR_UNSUPPORTED;
	if (p[5] < 1 || p[5] > 6 || p[6] || p[7] ||
		p[52] || p[53] || p[54] || p[55])
		return SYNC_ERR_PROTOCOL;
	sync_frame_info_t info = { .protocol_version = 1, .kind = p[5] };
	memcpy(info.session_id.bytes, p + 8, 16);
	struct sync_reader r = { .data = p + 24, .size = 28 };
	info.stream_id = sync_read_number(&r, 8);
	info.sequence = sync_read_number(&r, 8);
	info.tick = sync_read_number(&r, 8);
	info.payload_bytes = (uint32_t)sync_read_number(&r, 4);
	if (!memcmp(info.session_id.bytes, (uint8_t[16]){0}, 16) ||
		!info.stream_id || !info.sequence || !info.tick ||
		size - SYNC_HEADER != info.payload_bytes)
		return SYNC_ERR_PROTOCOL;
	*out_info = info;
	return SYNC_OK;
}
