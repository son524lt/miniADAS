import serial
import struct
import tkinter as tk
from tkinter import Canvas
import threading
import math

# ============ CONFIGURATION VARIABLES ============
COM_PORT = "COM10"
BAUDRATE = 921600
PACKET_START = bytes([0xAF, 0xFA, 0x55])
PACKET_END = bytes([0x77, 0xAA])
SPEED_ENDIAN = '<'
# ==================================================

# GUI Configuration
WINDOW_WIDTH = 1600
WINDOW_HEIGHT = 900
BG_COLOR = "#1e1e1e"
GAUGE_COLOR = "#2d2d2d"
TEXT_COLOR = "#ffffff"
ACCENT_COLOR = "#00ff00"


sound_speed = 343.0  # Speed of sound in m/s at room temperature
def getDistance(ms):
    return ((ms * 1e-4) * sound_speed/2 - 7) # Convert microseconds to distance in cm

class DataPacketParser:
    def __init__(self, port=COM_PORT, baudrate=BAUDRATE):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.running = False
        self.data = {
            'steering': 128,
            'user_throttle': 128,
            'true_throttle': 128,
            'brake': 0,
            'speed': 0,
            'PWM1': 0,
            'PWM2': 0,
            'distance': 0,
            'sample_rate': 0
        }
        self.lock = threading.Lock()
    
    def start(self):
        """Start serial reading thread"""
        try:
            print(f"Attempting to connect to {self.port} at {self.baudrate} baud...")
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            self.running = True
            thread = threading.Thread(target=self._read_thread, daemon=True)
            thread.start()
            print(f"✓ Connected to {self.port} at {self.baudrate} baud")
        except Exception as e:
            print(f"✗ Error connecting: {e}")
            print(f"  Please check if {self.port} is available")
    
    def _read_thread(self):
        """Read serial data in background - exact logic from read_pack.py"""
        while self.running:
            try:
                # Find packet start marker
                byte1 = self.ser.read(1)
                if not byte1:
                    continue
                
                if byte1[0] == PACKET_START[0]:
                    byte2 = self.ser.read(1)
                    if not byte2 or byte2[0] != PACKET_START[1]:
                        continue
                    
                    byte3 = self.ser.read(1)
                    if not byte3 or byte3[0] != PACKET_START[2]:
                        continue
                    
                    # Found valid start, read 13 bytes of data
                    data_bytes = self.ser.read(13)
                    if len(data_bytes) < 13:
                        continue
                    
                    # Read 2 bytes end marker
                    end_bytes = self.ser.read(2)
                    if len(end_bytes) < 2:
                        continue
                    
                    # Verify end marker
                    if end_bytes != PACKET_END:
                        continue
                    
                    # Parse data according to DataPacket struct
                    steering = data_bytes[0]
                    user_throttle = data_bytes[1]
                    true_throttle = data_bytes[2]
                    brake = data_bytes[3]
                    # speed: 4 bytes as int32_t (little-endian)
                    speed = struct.unpack(f'{SPEED_ENDIAN}i', data_bytes[4:8])[0]
                    pwm1 = data_bytes[8]
                    pwm2 = data_bytes[9]
                    # distance: 2 bytes as uint16_t (little-endian)
                    distance = struct.unpack(f'{SPEED_ENDIAN}H', data_bytes[10:12])[0]
                    sample_rate = data_bytes[12]
                    
                    # print(f"Packet: st={steering:3d}, ut={user_throttle:3d}, tt={true_throttle:3d}, br={brake:3d}, sp={speed:6d}, p1={pwm1:3d}, p2={pwm2:3d}, di={distance:5d}, sr={sample_rate:3d}")
                    
                    with self.lock:
                        self.data['steering'] = steering
                        self.data['user_throttle'] = user_throttle
                        self.data['true_throttle'] = true_throttle
                        self.data['brake'] = brake
                        self.data['speed'] = speed
                        self.data['PWM1'] = pwm1
                        self.data['PWM2'] = pwm2
                        self.data['distance'] = distance
                        self.data['sample_rate'] = sample_rate
            except Exception as e:
                pass  # Silent fail for robust background reading
    
    def get_data(self):
        """Get current data (thread-safe)"""
        with self.lock:
            return self.data.copy()
    
    def stop(self):
        """Stop serial reading"""
        self.running = False
        if self.ser:
            self.ser.close()


class VisualizerGUI:
    def __init__(self, root, parser):
        self.root = root
        self.parser = parser
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}+0+0")
        self.root.title("miniADAS Visualizer")
        self.root.configure(bg=BG_COLOR)
        
        # Create main canvas
        self.canvas = Canvas(root, width=WINDOW_WIDTH, height=WINDOW_HEIGHT, 
                           bg=BG_COLOR, highlightthickness=0)
        self.canvas.pack()
        
        # Start update loop
        self.update_display()
    
    def draw_gauge(self, x, y, radius, value, min_val=0, max_val=255, label="", unit=""):
        """Draw a circular gauge"""
        # Background circle
        self.canvas.create_oval(x-radius, y-radius, x+radius, y+radius, 
                              fill=GAUGE_COLOR, outline=ACCENT_COLOR, width=2)
        
        # Value text
        percent = (value - min_val) / (max_val - min_val) if max_val != min_val else 0
        percent = max(0, min(1, percent))
        angle = percent * 270 - 135  # 270 degree sweep from -135 to 135
        
        text = f"{value}"
        self.canvas.create_text(x, y, text=text, font=("Arial", 20, "bold"), 
                              fill=ACCENT_COLOR)
        self.canvas.create_text(x, y+35, text=label, font=("Arial", 12), 
                              fill=TEXT_COLOR)
        if unit:
            self.canvas.create_text(x, y+50, text=unit, font=("Arial", 10), 
                                  fill=TEXT_COLOR)
    
    def draw_horizontal_bar(self, x, y, width, height, value, min_val=0, max_val=255, label=""):
        """Draw a horizontal bar gauge"""
        # Background
        self.canvas.create_rectangle(x, y, x+width, y+height, fill=GAUGE_COLOR, 
                                   outline=ACCENT_COLOR, width=2)
        
        # Filled bar
        filled_width = (value - min_val) / (max_val - min_val) * width if max_val != min_val else 0
        filled_width = max(0, min(filled_width, width))
        self.canvas.create_rectangle(x, y, x+filled_width, y+height, 
                                   fill=ACCENT_COLOR, outline="")
        
        # Label and value
        self.canvas.create_text(x-100, y+height/2, text=label, font=("Arial", 12), 
                              fill=TEXT_COLOR, anchor="e")
        self.canvas.create_text(x+width+20, y+height/2, text=f"{value}", 
                              font=("Arial", 12, "bold"), fill=ACCENT_COLOR, anchor="w")
    
    def draw_steering_gauge(self, x, y, steering):
        """Draw steering angle gauge (horizontal)"""
        angle = int(round((steering / 255) * 120 - 60))  # Map 0-255 to -60 to +60 degrees
        
        # Background bar
        self.canvas.create_rectangle(x-150, y-30, x+150, y+30, fill=GAUGE_COLOR, 
                                   outline=ACCENT_COLOR, width=2)
        
        # Center line
        self.canvas.create_line(x, y-25, x, y+25, fill=TEXT_COLOR, width=1)
        
        # Needle position
        needle_x = x + (angle / 60) * 140
        self.canvas.create_line(x, y, needle_x, y, fill=ACCENT_COLOR, width=4)
        self.canvas.create_oval(needle_x-5, y-5, needle_x+5, y+5, fill=ACCENT_COLOR)
        
        # Labels
        self.canvas.create_text(x-140, y-40, text="-60°", font=("Arial", 10), 
                              fill=TEXT_COLOR)
        self.canvas.create_text(x, y-50, text="STEERING", font=("Arial", 12, "bold"), 
                              fill=ACCENT_COLOR)
        self.canvas.create_text(x+140, y-40, text="+60°", font=("Arial", 10), 
                              fill=TEXT_COLOR)
        self.canvas.create_text(x, y+50, text=f"{angle}°", font=("Arial", 14, "bold"), 
                              fill=ACCENT_COLOR)
    
    def update_display(self):
        """Update display with current data"""
        self.canvas.delete("all")
        
        data = self.parser.get_data()
        
        # Title
        self.canvas.create_text(WINDOW_WIDTH/2, 30, text="miniADAS Real-time Dashboard", 
                              font=("Arial", 24, "bold"), fill=ACCENT_COLOR)
        
        # Left column - Steering & Speed
        self.draw_steering_gauge(300, 150, data['steering'])
        
        # Speed gauge
        self.canvas.create_text(300, 250, text="SPEED", font=("Arial", 14, "bold"), 
                              fill=ACCENT_COLOR)
        self.draw_gauge(300, 350, 60, data['speed'], min_val=-1000, max_val=1000, 
                       label="km/h", unit="")
        
        # Sample rate & Distance
        self.canvas.create_text(300, 480, 
                              text=f"Sample Rate: {data['sample_rate']} Hz\nDistance: {data['distance']} us ({getDistance(data['distance']):.1f} cm)", 
                              font=("Arial", 12), fill=TEXT_COLOR, justify="center")
        
        # Middle column - Throttle & Brake
        y_start = 100
        self.canvas.create_text(WINDOW_WIDTH/2, y_start, text="THROTTLE & CONTROL", 
                              font=("Arial", 14, "bold"), fill=ACCENT_COLOR)
        
        # User throttle
        self.draw_horizontal_bar(WINDOW_WIDTH/2 - 100, y_start+40, 200, 30, 
                               data['user_throttle'], label="User Throttle")
        
        # True throttle
        self.draw_horizontal_bar(WINDOW_WIDTH/2 - 100, y_start+90, 200, 30, 
                               data['true_throttle'], label="True Throttle")
        
        # Brake
        self.draw_horizontal_bar(WINDOW_WIDTH/2 - 100, y_start+140, 200, 30, 
                               data['brake'], label="Brake")
        
        # PWM outputs
        self.draw_horizontal_bar(WINDOW_WIDTH/2 - 100, y_start+190, 200, 30, 
                               data['PWM1'], label="PWM1")
        
        self.draw_horizontal_bar(WINDOW_WIDTH/2 - 100, y_start+240, 200, 30, 
                               data['PWM2'], label="PWM2")
        
        # Right column - Data display
        right_x = WINDOW_WIDTH - 250
        self.canvas.create_text(right_x, 100, text="TELEMETRY", 
                              font=("Arial", 14, "bold"), fill=ACCENT_COLOR)
        
        telemetry_text = f"""
Steering:       {data['steering']:3d}
User Throttle:  {data['user_throttle']:3d}
True Throttle:  {data['true_throttle']:3d}
Brake:          {data['brake']:3d}
Speed:          {data['speed']:6d}
PWM1:           {data['PWM1']:3d}
PWM2:           {data['PWM2']:3d}
Distance:       {data['distance']:5d}
Sample Rate:    {data['sample_rate']:3d}
        """
        
        self.canvas.create_text(right_x, 250, text=telemetry_text, 
                              font=("Courier", 11), fill=TEXT_COLOR, justify="left")
        
        # Status bar
        status = f"Connected: {COM_PORT} @ {BAUDRATE} baud"
        self.canvas.create_rectangle(0, WINDOW_HEIGHT-30, WINDOW_WIDTH, WINDOW_HEIGHT, 
                                   fill=GAUGE_COLOR, outline=ACCENT_COLOR)
        self.canvas.create_text(10, WINDOW_HEIGHT-15, text=status, 
                              font=("Arial", 10), fill=TEXT_COLOR, anchor="w")
        
        # Schedule next update
        self.root.after(50, self.update_display)


def main():
    print("=" * 50)
    print("miniADAS Visualizer - Starting")
    print("=" * 50)
    
    parser = DataPacketParser()
    parser.start()
    
    root = tk.Tk()
    gui = VisualizerGUI(root, parser)
    
    def on_close():
        print("\nShutting down...")
        parser.stop()
        root.destroy()
    
    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
