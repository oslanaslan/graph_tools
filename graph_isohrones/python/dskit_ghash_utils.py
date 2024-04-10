# -*- coding: utf-8 -*-
import itertools
from math import floor


def mp_ghash(x, precision, bits_per_char):
    """Pyspark mapPartitions geohash retrieval from lat/lon"""

    def _rotate(n, x, y, rx, ry):
        if ry == 0:
            if rx == 1:
                x = n - 1 - x
                y = n - 1 - y
            return y, x
        return x, y

    def _xy2hash(x, y, dim):
        d = 0
        lvl = dim >> 1
        while lvl > 0:
            rx = int((x & lvl) > 0)
            ry = int((y & lvl) > 0)
            d += lvl * lvl * ((3 * rx) ^ ry)
            x, y = _rotate(lvl, x, y, rx, ry)
            lvl >>= 1
        return d

    def _coord2int(lng, lat, dim):
        _LAT_INTERVAL = (-90.0, 90.0)
        _LNG_INTERVAL = (-180.0, 180.0)
        lat_y = (lat + _LAT_INTERVAL[1]) / 180.0 * dim
        lng_x = (lng + _LNG_INTERVAL[1]) / 360.0 * dim

        return min(dim - 1, int(floor(lng_x))), min(dim - 1, int(floor(lat_y)))

    def _encode_int4(code):
        _BASE4 = "0123"
        code_len = (code.bit_length() + 1) // 2  # two bit per code point
        res = ["0"] * code_len

        for i in range(code_len - 1, -1, -1):
            res[i] = _BASE4[code & 0b11]
            code >>= 2

        return ''.join(res)

    def encode(lng, lat, precision=precision, bits_per_char=bits_per_char):
        bits = precision * bits_per_char
        level = bits >> 1
        dim = 1 << level

        x, y = _coord2int(lng, lat, dim)
        code = _xy2hash(x, y, dim)

        return _encode_int4(code).rjust(precision, "0")

    add_fields = ("uid", "uid_type", "dt")

    for row in x:
        lat = row["lat"]
        lon = row["lon"]
        ghash = encode(lng=lon, lat=lat, precision=precision, bits_per_char=bits_per_char)

        return_fields = [row[f] for f in add_fields]
        return_fields.extend([int(ghash)])

        yield tuple(return_fields)

def mp_ghash_v2(x, precision=16, base_columns=None):
    """Pyspark mapPartitions geohash retrieval from lat/lon"""
    from math import floor

    def _rotate(n, x, y, rx, ry):
        if ry == 0:
            if rx == 1:
                x = n - 1 - x
                y = n - 1 - y
            return y, x
        return x, y

    def _xy2hash(x, y, dim):
        d = 0
        lvl = dim >> 1
        while lvl > 0:
            rx = int((x & lvl) > 0)
            ry = int((y & lvl) > 0)
            d += lvl * lvl * ((3 * rx) ^ ry)
            x, y = _rotate(lvl, x, y, rx, ry)
            lvl >>= 1
        return d

    def _coord2int(lng, lat, dim):
        _LAT_INTERVAL = (-90.0, 90.0)
        _LNG_INTERVAL = (-180.0, 180.0)
        lat_y = (lat + _LAT_INTERVAL[1]) / 180.0 * dim
        lng_x = (lng + _LNG_INTERVAL[1]) / 360.0 * dim

        return min(dim - 1, int(floor(lng_x))), min(dim - 1, int(floor(lat_y)))

    def _encode_int4(code):
        _BASE4 = '0123'
        code_len = (code.bit_length() + 1) // 2  # two bit per code point
        res = ['0'] * code_len

        for i in range(code_len - 1, -1, -1):
            res[i] = _BASE4[code & 0b11]
            code >>= 2

        return ''.join(res)

    def encode(lng, lat, precision=18, bits_per_char=2):
        bits = precision * bits_per_char
        level = bits >> 1
        dim = 1 << level

        x, y = _coord2int(lng, lat, dim)
        code = _xy2hash(x, y, dim)

        return _encode_int4(code).rjust(precision, '0')

    if not base_columns:
        base_columns = []

    for row in x:
        lat = row['lat']
        lon = row['lon']
        ghash = encode(lon, lat, precision=precision, bits_per_char=2)

        return_fields = [row[column] for column in base_columns]
        return_fields += [lat, lon, ghash]

        yield tuple(return_fields)


def decode_exactly(code, bits_per_char=6):
    _LAT_INTERVAL = (-90.0, 90.0)
    _LNG_INTERVAL = (-180.0, 180.0)

    def _rotate(n, x, y, rx, ry):
        if ry == 0:
            if rx == 1:
                x = n - 1 - x
                y = n - 1 - y
            return y, x
        return x, y

    def _decode_int4(t):
        if len(t) == 0:
            return 0
        return int(t, 4)

    def decode_int(tag, bits_per_char=2):
        return _decode_int4(tag)

    def _hash2xy(hashcode, dim):
        x = y = 0
        lvl = 1
        while lvl < dim:
            rx = 1 & (hashcode >> 1)
            ry = 1 & (hashcode ^ rx)
            x, y = _rotate(lvl, x, y, rx, ry)
            x += lvl * rx
            y += lvl * ry
            hashcode >>= 2
            lvl <<= 1
        return x, y

    def _int2coord(x, y, dim):
        lng = float(x) / dim * 360 - 180
        lat = float(y) / dim * 180 - 90
        return lng, lat

    def _lvl_error(level):
        error = 1 / (1 << level)
        return 180 * error, 90 * error

    if len(code) == 0:
        return 0., 0., _LNG_INTERVAL[1], _LAT_INTERVAL[1]

    bits = len(code) * bits_per_char
    level = bits >> 1
    dim = 1 << level

    code_int = decode_int(code, bits_per_char)
    x, y = _hash2xy(code_int, dim)

    lng, lat = _int2coord(x, y, dim)
    lng_err, lat_err = _lvl_error(level)

    return lng + lng_err, lat + lat_err, lng_err, lat_err


def decode(code):
    if len(code) == 0:
        return [0., 0.]

    lng, lat, _lng_err, _lat_err = decode_exactly(code, 2)
    return [lng, lat]


def rectangle(code, bits_per_char=2):
    lng, lat, lng_err, lat_err = decode_exactly(code, bits_per_char)

    return [
        [lng - lng_err, lat - lat_err],
        [lng + lng_err, lat - lat_err],
        [lng + lng_err, lat + lat_err],
        [lng - lng_err, lat + lat_err],
        [lng - lng_err, lat - lat_err],
    ]


def encode(coordinates, precision=9):
    _base32 = '0123456789bcdefghjkmnpqrstuvwxyz'

    def _float_hex_to_int(f):
        if f < -1.0 or f >= 1.0:
            return None

        if f == 0.0:
            return 1, 1

        h = f.hex()
        x = h.find("0x1.")
        assert (x >= 0)
        p = h.find("p")
        assert (p > 0)

        half_len = len(h[x + 4:p]) * 4 - int(h[p + 1:])
        if x == 0:
            r = (1 << half_len) + ((1 << (len(h[x + 4:p]) * 4)) + int(h[x + 4:p], 16))
        else:
            r = (1 << half_len) - ((1 << (len(h[x + 4:p]) * 4)) + int(h[x + 4:p], 16))

        return r, half_len + 1

    def _encode_i2c(lat, lon, lat_length, lon_length):
        precision = int((lat_length + lon_length) / 5)
        if lat_length < lon_length:
            a = lon
            b = lat
        else:
            a = lat
            b = lon

        boost = (0, 1, 4, 5, 16, 17, 20, 21)
        ret = ''
        for i in range(precision):
            ret += _base32[(boost[a & 7] + (boost[b & 3] << 1)) & 0x1F]
            t = a >> 3
            a = b >> 2
            b = t

        return ret[::-1]

    longitude, latitude = coordinates
    xprecision = precision + 1
    lat_length = lon_length = int(xprecision * 5 / 2)
    if xprecision % 2 == 1:
        lon_length += 1

    if hasattr(float, "fromhex"):
        a = _float_hex_to_int(latitude / 90.0)
        o = _float_hex_to_int(longitude / 180.0)
        if a[1] > lat_length:
            ai = a[0] >> (a[1] - lat_length)
        else:
            ai = a[0] << (lat_length - a[1])

        if o[1] > lon_length:
            oi = o[0] >> (o[1] - lon_length)
        else:
            oi = o[0] << (lon_length - o[1])

        return _encode_i2c(ai, oi, lat_length, lon_length)[:precision]

    lat = latitude / 180.0
    lon = longitude / 360.0

    if lat > 0:
        lat = int((1 << lat_length) * lat) + (1 << (lat_length - 1))
    else:
        lat = (1 << lat_length - 1) - int((1 << lat_length) * (-lat))

    if lon > 0:
        lon = int((1 << lon_length) * lon) + (1 << (lon_length - 1))
    else:
        lon = (1 << lon_length - 1) - int((1 << lon_length) * (-lon))

    return _encode_i2c(lat, lon, lat_length, lon_length)[:precision]


def decode_2_to_coords_encode_to_5(x, base_columns=None):
    for row in x:
        code = row['ghash_2']
        res = decode(code)
        lon, lat = res
        res = encode((lon, lat))

        return_fields = [row[f] for f in base_columns]

        return_fields.extend([lon, lat])
        return_fields.extend([res])

        yield tuple(return_fields)
