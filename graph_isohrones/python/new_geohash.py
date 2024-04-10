import typing as tp

import numpy as np


_LAT_INTERVAL = (-90.0, 90.0)
_LNG_INTERVAL = (-180.0, 180.0)


# custom base64 encoding with integer order preservation via lexicographical (byte) order.
_BASE64 = (
    "0123456789"  # noqa: E262    #   10    0x30 - 0x39
    "@"  # +  1    0x40
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  # + 26    0x41 - 0x5A
    "_"  # +  1    0x5F
    "abcdefghijklmnopqrstuvwxyz"  # + 26    0x61 - 0x7A
)  # = 64    0x30 - 0x7A

_BASE16 = "0123456789abcdef"


def encode(lng: tp.Any, lat: tp.Any, precision: int = 18, bits_per_char: int = 2) -> np.ndarray:
    """
    Encode a lng/lat position as a geohash using a hilbert curve

    This function encodes a lng/lat coordinate to a geohash of length `precision`
    on a corresponding a hilbert curve. Each character encodes `bits_per_char` bits
    per character (allowed are 2, 4 and 6 bits [default 6]). Hence, the geohash encodes
    the lng/lat coordinate using `precision` * `bits_per_char` bits. The number of
    bits devided by 2 give the level of the used hilbert curve, e.g. precision=10, bits_per_char=6
    (default values) use 60 bit and a level 30 hilbert curve to map the globe.

    Parameters:
        lng: numpy array-like    Longitude; between -180.0 and 180.0; WGS 84
        lat: numpy array-like    Latitude; between -90.0 and 90.0; WGS 84
        precision: int           The number of characters in a geohash
        bits_per_char: int       The number of bits per coding character

    Returns:
        Array of dtype str. Geohashes for lng/lat of length `precision`
    """
    lng = np.asarray(lng, dtype=np.float64)
    lat = np.asarray(lat, dtype=np.float64)

    if not ((_LNG_INTERVAL[0] <= lng) & (lng <= _LNG_INTERVAL[1])).all():
        raise ValueError(f"Longitude should be between {_LNG_INTERVAL[0]} and {_LNG_INTERVAL[1]}.")
    if not ((_LAT_INTERVAL[0] <= lat) & (lat <= _LAT_INTERVAL[1])).all():
        raise ValueError(f"Latitude should be between {_LAT_INTERVAL[0]} and {_LAT_INTERVAL[1]}.")
    if precision <= 0:
        raise ValueError("`precision` should be positive,")
    if bits_per_char not in (2, 4, 6):
        raise ValueError("`bits_per_char` should be in {6, 4, 2}")

    bits = precision * bits_per_char
    if bits > np.iinfo(np.uint64).bits:
        raise ValueError(
            f"`precision` * `bits_per_char` cannot exceed {np.iinfo(np.uint64).bits} "
            "due to implementation specifics."
        )

    level = bits >> 1
    dim = 1 << level

    x, y = _coord2int(lng, lat, dim)

    code = _xy2hash(x, y, dim)

    return np.char.rjust(encode_int(code, bits_per_char), precision, "0")


def encode_int(code: np.ndarray, bits_per_char: int = 6) -> np.ndarray:
    """
    Encode int into a string preserving order

    It is using 2, 4 or 6 bits per coding character (default 6).

    Parameters:
        code: int           Positive integer.
        bits_per_char: int  The number of bits per coding character.

    Returns:
        str: the encoded integer
    """
    if code.any() < 0:
        raise ValueError("Only positive ints are allowed!")

    if bits_per_char == 6:
        return _encode_int(code, bits_per_char, _BASE64)
    if bits_per_char == 4:
        return _encode_int(code, bits_per_char, _BASE16)
    if bits_per_char == 2:
        try:
            encoded = _encode_int_as_decimal(code, bits_per_char).astype("U")
        except ValueError:
            encoded = _encode_int(code, bits_per_char, _BASE16[:4])
        return encoded

    raise ValueError("`bits_per_char` must be in {6, 4, 2}")


def _encode_int(code: np.ndarray, bits_per_char: int, alphabet: str) -> np.ndarray:
    alphabet = np.array(list(alphabet), dtype="U1")
    mask = (1 << bits_per_char) - 1  # e.g., 0b111111 if bits_per_char=6
    code_len = (int(code.max()).bit_length() + bits_per_char - 1) // bits_per_char
    bit_shift = bits_per_char * np.arange(code_len, dtype=np.uint64)
    bit_shift = bit_shift[(None,) * code.ndim + (...,)]  # match the code shape
    code = code[..., None]  # match the bit_shift shape
    shifted_codes = code >> bit_shift
    encoded = alphabet[shifted_codes & mask][..., ::-1]
    return np.apply_along_axis("".join, -1, encoded)


def _encode_int_as_decimal(code: np.ndarray, bits_per_char: int) -> np.ndarray:
    """
    Convert the given integer to an integer whose decimal representation coincides with the encoding.
    """
    if bits_per_char > 3:
        raise ValueError("Only `bits_per_char` <=3 supported.")
    mask = (1 << bits_per_char) - 1  # e.g., 0b11 if bits_per_char=2
    code_len = (int(code.max()).bit_length() + bits_per_char - 1) // bits_per_char

    if mask * (10 ** (code_len - 1)) > np.iinfo(np.uint64).max:
        raise ValueError(f"The integers are too large to be encoded in uint64.")

    code_len_range = np.arange(code_len, dtype=np.uint64)
    code_len_range = code_len_range[(None,) * code.ndim + (...,)]  # match the code shape
    bit_shift = bits_per_char * code_len_range
    shifted_codes = code[..., None] >> bit_shift
    decimal_code = (shifted_codes & mask) * (10**code_len_range)
    return decimal_code.sum(axis=-1)


def _coord2int(lng: np.ndarray, lat: np.ndarray, dim: int) -> tp.Tuple[np.ndarray, np.ndarray]:
    """
    Convert lon, lat values into a (dim x dim)-grid coordinate system.

    Parameters:
        lng: array of dtype float64    Longitude value of coordinate (-180.0, 180.0); corresponds to X axis
        lat: array of dtype float64    Latitude value of coordinate (-90.0, 90.0); corresponds to Y axis
        dim: int                       Number of coding points each x, y value can take.
                                       Corresponds to 2^level of the hilbert curve.

    Returns:
        Lower left corner of corresponding (dim x dim)-grid box
          x: array of dtype uint64     x values of point [0, dim); corresponds to longitude
          y: array of dtype uint64     y values of point [0, dim); corresponds to latitude
    """
    assert dim >= 1
    assert dim <= np.iinfo(np.uint64).max

    lng_x = (lng + _LNG_INTERVAL[1]) / (_LNG_INTERVAL[1] - _LNG_INTERVAL[0])  # [0, 1]
    lat_y = (lat + _LAT_INTERVAL[1]) / (_LAT_INTERVAL[1] - _LAT_INTERVAL[0])  # [0, 1]

    xy = (
        np.minimum(dim - 1, np.floor(lng_x * dim).astype(np.uint64), dtype=np.uint64),
        np.minimum(dim - 1, np.floor(lat_y * dim).astype(np.uint64), dtype=np.uint64),
    )

    return xy


def _xy2hash(x: np.ndarray, y: np.ndarray, dim: int) -> np.ndarray:
    """
    Convert (x, y) to hashcode.

    Based on the implementation here:
        https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503

    Parameters:
        x: array of dtype uint64    x values of point [0, dim) in (dim x dim) coord system
        y: array of dtype uint64    y values of point [0, dim) in (dim x dim) coord system
        dim: int                    Number of coding points each x, y value can take.
                                    Corresponds to 2^level of the hilbert curve.

    Returns:
        Array of dtype uint64. Hashcode in [0, dim**2)
    """
    d = np.zeros_like(x ^ y)
    lvl = dim >> 1
    while lvl > 0:
        rx = ((x & lvl) > 0).astype(np.uint64)
        ry = ((y & lvl) > 0).astype(np.uint64)
        d += lvl * lvl * ((3 * rx) ^ ry)
        x, y = _rotate(lvl, x, y, rx, ry)
        lvl >>= 1
    return d


def _rotate(n, x: np.ndarray, y: np.ndarray, rx: np.ndarray, ry: np.ndarray) -> np.ndarray:
    """
    Rotate and flip a quadrant appropriately.

    Based on the implementation here:
        https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503

    Parameters:
        x: array of dtype uint64
        y: array of dtype uint64
        rx: array of dtype uint64
        ry: array of dtype uint64

    Returns:
        x: array of dtype uint64    Rotated x
        y: array of dtype uint64    Rotated y

    """
    xy = np.stack([x, y])
    swap = ry == 0
    flip = swap & (rx == 1)
    xy[:, swap] = xy[::-1, swap]
    xy[:, flip] = n - 1 - xy[:, flip]
    x, y = xy
    return x, y
