# -*- coding: utf-8 -*-
from __future__ import absolute_import, division, print_function, unicode_literals

import os
import sys

from math import floor

sys.path.append(os.path.dirname(__file__))

try:
    from cgeohash import hash2xy_cython, MAX_BITS, xy2hash_cython

    CYTHON_AVAILABLE = True
except ImportError:
    CYTHON_AVAILABLE = False

_LAT_INTERVAL = (-90.0, 90.0)
_LNG_INTERVAL = (-180.0, 180.0)


def encode_int(code, bits_per_char=6):
    """Encode int into a string preserving order

    It is using 2, 4 or 6 bits per coding character (default 6).

    Parameters:
        code: int           Positive integer.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        str: the encoded integer
    """
    if code < 0:
        raise ValueError('Only positive ints are allowed!')

    if bits_per_char == 6:
        return _encode_int64(code)
    if bits_per_char == 4:
        return _encode_int16(code)
    if bits_per_char == 2:
        return _encode_int4(code)

    raise ValueError('`bits_per_char` must be in {6, 4, 2}')


def decode_int(tag, bits_per_char=6):
    """Decode string into int assuming encoding with `encode_int()`

    It is using 2, 4 or 6 bits per coding character (default 6).

    Parameters:
        tag: str           Encoded integer.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        int: the decoded string
    """
    if bits_per_char == 6:
        return _decode_int64(tag)
    if bits_per_char == 4:
        return _decode_int16(tag)
    if bits_per_char == 2:
        return _decode_int4(tag)

    raise ValueError('`bits_per_char` must be in {6, 4, 2}')


# Own base64 encoding with integer order preservation via lexicographical (byte) order.
_BASE64 = (
    '0123456789'  # noqa: E262    #   10    0x30 - 0x39
    '@'  # +  1    0x40
    'ABCDEFGHIJKLMNOPQRSTUVWXYZ'  # + 26    0x41 - 0x5A
    '_'  # +  1    0x5F
    'abcdefghijklmnopqrstuvwxyz'  # + 26    0x61 - 0x7A
)  # = 64    0x30 - 0x7A
_BASE64_MAP = {c: i for i, c in enumerate(_BASE64)}


def _encode_int64(code):
    code_len = (code.bit_length() + 5) // 6  # 6 bit per code point
    res = ['0'] * code_len
    for i in range(code_len - 1, -1, -1):
        res[i] = _BASE64[code & 0b111111]
        code >>= 6
    return ''.join(res)


def _decode_int64(t):
    code = 0
    for ch in t:
        code <<= 6
        code += _BASE64_MAP[ch]
    return code


def _encode_int16(code):
    code = '' + hex(code)[2:]  # this makes it unicode in py2
    if code.endswith('L'):
        code = code[:-1]
    return code


def _decode_int16(t):
    if len(t) == 0:
        return 0
    return int(t, 16)


def _encode_int4(code):
    _BASE4 = '0123'
    code_len = (code.bit_length() + 1) // 2  # two bit per code point
    res = ['0'] * code_len

    for i in range(code_len - 1, -1, -1):
        res[i] = _BASE4[code & 0b11]
        code >>= 2

    return ''.join(res)


def _decode_int4(t):
    if len(t) == 0:
        return 0
    return int(t, 4)


def neighbours(code, bits_per_char=6):
    """Get the neighbouring geohashes for `code`.

    Look for the north, north-east, east, south-east, south, south-west, west,
    north-west neighbours. If you are at the east/west edge of the grid
    (lng ∈ (-180, 180)), then it wraps around the globe and gets the corresponding
    neighbor.

    Parameters:
        code: str           The geohash at the center.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        dict: geohashes in the neighborhood of `code`. Possible keys are 'north',
            'north-east', 'east', 'south-east', 'south', 'south-west',
            'west', 'north-west'. If the input code covers the north pole, then
            keys 'north', 'north-east', and 'north-west' are not present, and if
            the input code covers the south pole then keys 'south', 'south-west',
            and 'south-east' are not present.
    """
    lng, lat, lng_err, lat_err = decode_exactly(code, bits_per_char)
    precision = len(code)

    north = lat + 2 * lat_err

    south = lat - 2 * lat_err

    east = lng + 2 * lng_err
    if east > 180:
        east -= 360

    west = lng - 2 * lng_err
    if west < -180:
        west += 360

    neighbours_dict = {
        'east': encode(east, lat, precision, bits_per_char),  # noqa: E241
        'west': encode(west, lat, precision, bits_per_char),  # noqa: E241
    }

    if north <= 90:  # input cell not already at the north pole
        neighbours_dict.update({
            'north': encode(lng, north, precision, bits_per_char),  # noqa: E241
            'north-east': encode(east, north, precision, bits_per_char),  # noqa: E241
            'north-west': encode(west, north, precision, bits_per_char),  # noqa: E241
        })

    if south >= -90:  # input cell not already at the south pole
        neighbours_dict.update({
            'south': encode(lng, south, precision, bits_per_char),  # noqa: E241
            'south-east': encode(east, south, precision, bits_per_char),  # noqa: E241
            'south-west': encode(west, south, precision, bits_per_char),  # noqa: E241
        })

    return neighbours_dict


def rectangle(code, bits_per_char=6):
    """Builds a (geojson) rectangle from `code`

    The center of the rectangle decodes as the lng/lat for code and
    the rectangle corresponds to the error-margin, i.e. every lng/lat
    point within this rectangle will be encoded as `code`, given `precision == len(code)`.

    Parameters:
        code: str           The geohash for which the rectangle should be build.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        dict: geojson `Feature` containing the rectangle as a `Polygon`.
    """
    lng, lat, lng_err, lat_err = decode_exactly(code, bits_per_char)

    return {
        'type': 'Feature',
        'properties': {
            'code': code,
            'lng': lng,
            'lat': lat,
            'lng_err': lng_err,
            'lat_err': lat_err,
            'bits_per_char': bits_per_char,
        },
        'bbox': (
            lng - lng_err,  # bottom left
            lat - lat_err,
            lng + lng_err,  # top right
            lat + lat_err,
        ),
        'geometry': {
            'type': 'Polygon',
            'coordinates': [[
                (lng - lng_err, lat - lat_err),
                (lng + lng_err, lat - lat_err),
                (lng + lng_err, lat + lat_err),
                (lng - lng_err, lat + lat_err),
                (lng - lng_err, lat - lat_err),
            ]],
        },
    }


def hilbert_curve(precision, bits_per_char=6):
    """Build the (geojson) `LineString` of the used hilbert-curve

    Builds the `LineString` of the used hilbert-curve given the `precision` and
    the `bits_per_char`. The number of bits to encode the geohash is equal to
    `precision * bits_per_char`, and for each level, you need 2 bits, hence
    the number of bits has to be even. The more bits are used, the more precise
    (and long) will the hilbert curve be, e.g. for geohashes of length 3 (precision)
    and 6 bits per character, there will be 18 bits used and the curve will
    consist of 2^18 = 262144 points.

    Parameters:
        precision: int      The number of characters in a geohash.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        dict: geojson `Feature` containing the hilbert curve as a `LineString`.
    """
    bits = precision * bits_per_char

    coords = []
    for i in range(1 << bits):
        code = encode_int(i, bits_per_char).rjust(precision, '0')
        coords += [decode(code, bits_per_char)]

    return {
        'type': 'Feature',
        'properties': {},
        'geometry': {
            'type': 'LineString',
            'coordinates': coords,
        },
    }


def encode(lng, lat, precision=10, bits_per_char=6):
    """Encode a lng/lat position as a geohash using a hilbert curve

    This function encodes a lng/lat coordinate to a geohash of length `precision`
    on a corresponding a hilbert curve. Each character encodes `bits_per_char` bits
    per character (allowed are 2, 4 and 6 bits [default 6]). Hence, the geohash encodes
    the lng/lat coordinate using `precision` * `bits_per_char` bits. The number of
    bits devided by 2 give the level of the used hilbert curve, e.g. precision=10, bits_per_char=6
    (default values) use 60 bit and a level 30 hilbert curve to map the globe.

    Parameters:
        lng: float          Longitude; between -180.0 and 180.0; WGS 84
        lat: float          Latitude; between -90.0 and 90.0; WGS 84
        precision: int      The number of characters in a geohash
        bits_per_char: int  The number of bits per coding character

    Returns:
        str: geohash for lng/lat of length `precision`
    """
    assert _LNG_INTERVAL[0] <= lng <= _LNG_INTERVAL[1]
    assert _LAT_INTERVAL[0] <= lat <= _LAT_INTERVAL[1]
    assert precision > 0
    assert bits_per_char in (2, 4, 6)

    bits = precision * bits_per_char
    level = bits >> 1
    dim = 1 << level

    x, y = _coord2int(lng, lat, dim)

    if CYTHON_AVAILABLE and bits <= MAX_BITS:
        code = xy2hash_cython(x, y, dim)
    else:
        code = _xy2hash(x, y, dim)

    return encode_int(code, bits_per_char).rjust(precision, '0')


def decode(code, bits_per_char=6):
    """Decode a geohash on a hilbert curve as a lng/lat position

    Decodes the geohash `code` as a lng/lat position. It assumes, that
    the length of `code` corresponds to the precision! And that each character
    in `code` encodes `bits_per_char` bits. Do not mix geohashes with different
    `bits_per_char`!

    Parameters:
        code: str           The geohash to decode.
        bits_per_char: int  The number of bits per coding character

    Returns:
        Tuple[float, float]:  (lng, lat) coordinate for the geohash.
    """
    assert bits_per_char in (2, 4, 6)

    if len(code) == 0:
        return 0., 0.

    lng, lat, _lng_err, _lat_err = decode_exactly(code, bits_per_char)
    return lng, lat


def decode_exactly(code, bits_per_char=6):
    """Decode a geohash on a hilbert curve as a lng/lat position with error-margins

    Decodes the geohash `code` as a lng/lat position with error-margins. It assumes,
    that the length of `code` corresponds to the precision! And that each character
    in `code` encodes `bits_per_char` bits. Do not mix geohashes with different
    `bits_per_char`!

    Parameters:
        code: str           The geohash to decode.
        bits_per_char: int  The number of bits per coding character

    Returns:
        Tuple[float, float, float, float]:  (lng, lat, lng-error, lat-error) coordinate for the geohash.
    """
    assert bits_per_char in (2, 4, 6)

    if len(code) == 0:
        return 0., 0., _LNG_INTERVAL[1], _LAT_INTERVAL[1]

    bits = len(code) * bits_per_char
    level = bits >> 1
    dim = 1 << level

    code_int = decode_int(code, bits_per_char)
    if CYTHON_AVAILABLE and bits <= MAX_BITS:
        x, y = hash2xy_cython(code_int, dim)
    else:
        x, y = _hash2xy(code_int, dim)

    lng, lat = _int2coord(x, y, dim)
    lng_err, lat_err = _lvl_error(level)  # level of hilbert curve is bits / 2

    return lng + lng_err, lat + lat_err, lng_err, lat_err


def _lvl_error(level):
    """Get the lng/lat error for the hilbert curve with the given level

    On every level, the error of the hilbert curve is halved, e.g.
    - level 0 has lng error of +-180 (only one coding point is available: (0, 0))
    - on level 1, there are 4 coding points: (-90, -45), (90, -45), (-90, 45), (90, 45)
      hence the lng error is +-90

    Parameters:
        level: int  Level of the used hilbert curve

    Returns:
        Tuple[float, float]: (lng-error, lat-error) for the given level
    """
    error = 1 / (1 << level)
    return 180 * error, 90 * error


def _coord2int(lng, lat, dim):
    """Convert lon, lat values into a dim x dim-grid coordinate system.

    Parameters:
        lng: float    Longitude value of coordinate (-180.0, 180.0); corresponds to X axis
        lat: float    Latitude value of coordinate (-90.0, 90.0); corresponds to Y axis
        dim: int      Number of coding points each x, y value can take.
                      Corresponds to 2^level of the hilbert curve.

    Returns:
        Tuple[int, int]:
            Lower left corner of corresponding dim x dim-grid box
              x      x value of point [0, dim); corresponds to longitude
              y      y value of point [0, dim); corresponds to latitude
    """
    assert dim >= 1

    lat_y = (lat + _LAT_INTERVAL[1]) / 180.0 * dim  # [0 ... dim)
    lng_x = (lng + _LNG_INTERVAL[1]) / 360.0 * dim  # [0 ... dim)

    return min(dim - 1, int(floor(lng_x))), min(dim - 1, int(floor(lat_y)))


def _int2coord(x, y, dim):
    """Convert x, y values in dim x dim-grid coordinate system into lng, lat values.

    Parameters:
        x: int        x value of point [0, dim); corresponds to longitude
        y: int        y value of point [0, dim); corresponds to latitude
        dim: int      Number of coding points each x, y value can take.
                      Corresponds to 2^level of the hilbert curve.

    Returns:
        Tuple[float, float]: (lng, lat)
            lng    longitude value of coordinate [-180.0, 180.0]; corresponds to X axis
            lat    latitude value of coordinate [-90.0, 90.0]; corresponds to Y axis
    """
    assert dim >= 1
    assert x < dim
    assert y < dim

    lng = x / dim * 360 - 180
    lat = y / dim * 180 - 90

    return lng, lat


# only use python versions, when cython is not available
def _xy2hash(x, y, dim):
    """Convert (x, y) to hashcode.

    Based on the implementation here:
        https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503

    Pure python implementation.

    Parameters:
        x: int        x value of point [0, dim) in dim x dim coord system
        y: int        y value of point [0, dim) in dim x dim coord system
        dim: int      Number of coding points each x, y value can take.
                      Corresponds to 2^level of the hilbert curve.

    Returns:
        int: hashcode  ∈ [0, dim**2)
    """
    d = 0
    lvl = dim >> 1
    while lvl > 0:
        rx = int((x & lvl) > 0)
        ry = int((y & lvl) > 0)
        d += lvl * lvl * ((3 * rx) ^ ry)
        x, y = _rotate(lvl, x, y, rx, ry)
        lvl >>= 1
    return d


def _hash2xy(hashcode, dim):
    """Convert hashcode to (x, y).

    Based on the implementation here:
        https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503

    Pure python implementation.

    Parameters:
        hashcode: int  Hashcode to decode [0, dim**2)
        dim: int       Number of coding points each x, y value can take.
                       Corresponds to 2^level of the hilbert curve.

    Returns:
        Tuple[int, int]: (x, y) point in dim x dim-grid system
    """
    assert (hashcode <= dim * dim - 1)
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


def _rotate(n, x, y, rx, ry):
    """Rotate and flip a quadrant appropriately

    Based on the implementation here:
        https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503

    """
    if ry == 0:
        if rx == 1:
            x = n - 1 - x
            y = n - 1 - y
        return y, x
    return x, y
