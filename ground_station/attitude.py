import numpy as np
from vispy.util.quaternion import Quaternion

data = [0]*4

def estimate(quat_packet, ns_qstate):
  """
  quat_packet: list of 8 raw bytes 
  convert to signed int16; converted to float32; update ns_qstate.heading;
  """
  for i in range(0,8,2):
    data[i//2] = (quat_packet[i]<<8)|quat_packet[i+1]
    if data[i//2] & 0x8000:
      data[i//2] = data[i//2] - 0x10000
  qq = np.array([float(data[0])/16384.0, float(data[1])/16384.0, float(data[2])/16384.0, float(data[3])/16384.0], dtype=np.float32)
  # quat_packet -> data -> quaternion
  # The order is weird, right?
  # That's because of the way GroundStation and Quadcopter Coordinate Systems are aligned
  # This weird order just converts rotation in Quad-system to GS-system
  ns_qstate.heading = Quaternion(qq[0], -qq[1], -qq[3], qq[2])
  print("wxyz", qq)