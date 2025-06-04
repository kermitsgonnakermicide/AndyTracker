import requests
import os
from dotenv import load_dotenv

# Load the .env file
load_dotenv()
api_token = os.getenv("API_TOKEN")

# Traccar demo server
base_url = "https://demo.traccar.org/api"
headers = {
    "Authorization": f"Bearer {api_token}"
}

# Step 1: Get all devices
devices_url = f"{base_url}/devices"
response = requests.get(devices_url, headers=headers)

if response.status_code == 200:
    devices = response.json()
    # Step 2: Find andyphone
    andy_device = next((d for d in devices if d['name'] == "andyphone"), None)
    
    if andy_device:
        device_id = andy_device['id']

        # Step 3: Get latest position for andyphone
        positions_url = f"{base_url}/positions?deviceId={device_id}"
        pos_response = requests.get(positions_url, headers=headers)
        
        if pos_response.status_code == 200 and pos_response.json():
            position = pos_response.json()[0]
            print(f"andyphone location:\nLatitude: {position['latitude']}\nLongitude: {position['longitude']}")
        else:
            print("Position not found for andyphone.")
    else:
        print("Device 'andyphone' not found.")
else:
    print(f"Failed to fetch devices. HTTP {response.status_code}")
