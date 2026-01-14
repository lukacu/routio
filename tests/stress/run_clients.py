import time
import multiprocessing
import routio
import random

def client_worker(client_id, all_channels, duration):

    loop = routio.IOLoop()

    client = routio.Client()
    loop.add_handler(client)
    
    # Set up publishers for each channel (each client sends to all channels)
    pubs = [routio.Publisher(client, channel, "i") for channel in all_channels]
    
    # Set up counters for received messages
    recv_count = 0

    # Callback for message reception
    def make_callback():
        def callback(msg):
            nonlocal recv_count
            recv_count += 1
        return callback

    # Subscribe to all channels with dummy callbacks
    subs = [routio.Subscriber(client, channel, "i", make_callback()) for channel in all_channels]

    sent_count = 0
    start_time = time.time()

    # Send messages repeatedly during the test window
    while time.time() - start_time < duration:
        for pub in pubs:
            writer = routio.MessageWriter()
            writer.writeString(f"client_{client_id}")
            # Generate random length payload for stress testing
            payload_length = random.randint(1, 10000)
            # Create a payload string of random length and random content
            payload_string = ''.join(random.choices('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789', k=payload_length))
            writer.writeString(payload_string)
            pub.send(writer)
            sent_count += 1

        loop.wait(50)
        
    loop.wait(1000)  # Let any remaining messages flush

    client.cl

    print(f"Client {client_id} sent {sent_count} messages, received ~{recv_count} messages")

def run_stress_test(num_clients=10, channels_per_client=2, duration=10):
    # Generate all unique channels
    all_channels = [f"channel_{i}" for i in range(channels_per_client)]

    processes = []
    for i in range(num_clients):
        p = multiprocessing.Process(target=client_worker, args=(i, all_channels, duration))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()

if __name__ == "__main__":
    run_stress_test(num_clients=30, channels_per_client=10, duration=5)
