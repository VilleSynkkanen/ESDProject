import time
import hike
import bt

hubbt = bt.HubBluetooth()

def process_sessions(sessions: list[hike.HikeSession]):
    """Callback function to process sessions.

    Saves the first session into a text file.

    Args:
        sessions: list of `hike.HikeSession` objects to process
    """
    print("Processing sessions")
    s = sessions[0]
    print(s)
    write_session(s.steps, s.m)

def write_session(steps, dist):
    f = open('data/session.txt', 'w')
    f.write("steps:" + str(steps) + "\n")
    f.write("dist:" + str(dist))
    f.close()

def main():
    print("Starting Bluetooth receiver.")
    try:
        while True:
            hubbt.wait_for_connection()
            hubbt.synchronize(callback=process_sessions)
            break
            
    except KeyboardInterrupt:
        print("CTRL+C Pressed. Shutting down the server...")

    except Exception as e:
        print(f"Unexpected shutdown...")
        print(f"ERROR: {e}")
        hubbt.sock.close()
        raise e

if __name__ == "__main__":
    main()