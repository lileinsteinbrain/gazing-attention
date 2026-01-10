import cv2
import numpy as np

cap = cv2.VideoCapture(0)

ret, prev = cap.read()
prev_gray = cv2.cvtColor(prev, cv2.COLOR_BGR2GRAY)

lambda_m = 0.0
alpha = 0.1  # smoothing

while True:
    ret, frame = cap.read()
    if not ret:
        break

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    diff = cv2.absdiff(gray, prev_gray)
    motion = np.mean(diff) / 255.0  # normalize 0–1

    # smooth → λₘ
    lambda_m = (1 - alpha) * lambda_m + alpha * motion

    # visualisation for portfolio
    cv2.putText(
        frame,
        f"lambda_m: {lambda_m:.3f}",
        (20, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 0, 255),
        2
    )

    cv2.imshow("LV1 Motion Proxy", frame)

    prev_gray = gray

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()
