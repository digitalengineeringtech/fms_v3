// Optimized script.js for ESP32 OTA Web UI
document.addEventListener("DOMContentLoaded", () => {
  // Cache DOM elements
  const elements = {
    deviceName: document.getElementById("device-name"),
    logout: document.getElementById("logout"),
    firmwareVersion: document.getElementById("firmware-version"),
    ipAddress: document.getElementById("ip-address"),
    macAddress: document.getElementById("mac-address"),
    wifiSignal: document.getElementById("wifi-signal"),
    uptime: document.getElementById("uptime"),
    freeMemory: document.getElementById("free-memory"),
    cpuFreq: document.getElementById("cpu-freq"),
    flashSize: document.getElementById("flash-size"),
    updateStatus: document.getElementById("update-status"),
    refreshBtn: document.getElementById("refresh-btn"),
    firmwareFile: document.getElementById("firmware-file"),
    fileName: document.getElementById("file-name"),
    updateForm: document.getElementById("update-form"),
    updateBtn: document.getElementById("update-btn"),
    restartBtn: document.getElementById("restart-btn"),
    progressContainer: document.getElementById("progress-container"),
    progressBarFill: document.getElementById("progress-bar-fill"),
    progressText: document.getElementById("progress-text"),
    progressPercentage: document.getElementById("progress-percentage"),
    modal: document.getElementById("modal"),
    modalTitle: document.getElementById("modal-title"),
    modalBody: document.getElementById("modal-body"),
    modalConfirm: document.getElementById("modal-confirm"),
    modalCancel: document.getElementById("modal-cancel"),
    modalClose: document.getElementById("modal-close"),
  }

  // State
  const state = {
    updateInProgress: false,
    infoRefreshInterval: null,
    updateCheckInterval: null,
    reconnectAttempts: 0,
    lastInfoUpdate: 0,
    deviceData: null,
    MAX_RECONNECT_ATTEMPTS: 30,
    REFRESH_INTERVAL: 5000, // 5 seconds
    THROTTLE_INTERVAL: 2000, // 2 seconds
  }

  // Initialize
  init()

  function init() {
    // Set up event listeners
    elements.refreshBtn.addEventListener("click", fetchDeviceInfo)
    elements.firmwareFile.addEventListener("change", handleFileSelect)
    elements.updateForm.addEventListener("submit", handleUpdateSubmit)
    elements.restartBtn.addEventListener("click", confirmRestart)
    elements.modalClose.addEventListener("click", closeModal)
    elements.modalCancel.addEventListener("click", closeModal)
    elements.logout.addEventListener("click", () => {
      showModal("Logout", "Are you sure you want to log out?", () => {
        fetch("/logout").then((response) => response.text()).then(data => { 
          console.log("Logout response:", data)
          window.location.href = "/"
        }
        ).catch(error => {
          console.error("Error during logout:", error)
          showModal("Logout Failed", "There was an error logging out. Please try again.")
        });
        // window.location.href = "/logout"
      })
    })
    // Initial data fetch
    fetchDeviceInfo()

    // Set up auto-refresh (throttled)
    state.infoRefreshInterval = setInterval(() => {
      // Only refresh if not in the middle of an update and if enough time has passed
      if (!state.updateInProgress && Date.now() - state.lastInfoUpdate > state.THROTTLE_INTERVAL) {
        fetchDeviceInfo()
      }
    }, state.REFRESH_INTERVAL)
  }

  function fetchDeviceInfo() {
    // Prevent excessive requests
    if (Date.now() - state.lastInfoUpdate < state.THROTTLE_INTERVAL) return

    state.lastInfoUpdate = Date.now()

    fetch("/api/info", {
      method: "GET",
      headers: { "Cache-Control": "no-cache" },
      timeout: 3000,
    })
      .then((response) => {
        if (!response.ok) throw new Error("Failed to fetch device info")
        state.reconnectAttempts = 0
        return response.json()
      })
      .then((data) => {
        state.deviceData = data
        updateDeviceInfo(data)
      })
      .catch((error) => {
        console.error("Error fetching device info:", error)
        handleConnectionError()
      })
  }

  function updateDeviceInfo(data) {
    // Update only if data has changed to reduce DOM operations
    if (!data) return

    elements.deviceName.textContent = data.deviceName || "ULTM_"
    elements.firmwareVersion.textContent = data.firmwareVersion || "Unknown"
    elements.ipAddress.textContent = data.ipAddress || "Unknown"
    elements.macAddress.textContent = data.macAddress || "Unknown"
    elements.wifiSignal.textContent = data.rssi ? `${data.rssi} dBm` : "Unknown"

    // Format uptime
    elements.uptime.textContent = formatUptime(data.uptime || 0)

    // Format memory
    const freeHeap = data.freeHeap || 0
    const totalHeap = data.totalHeap || 0
    const usedPercentage = totalHeap > 0 ? Math.round((1 - freeHeap / totalHeap) * 100) : 0
    elements.freeMemory.textContent = `${formatBytes(freeHeap)} (${usedPercentage}% used)`

    elements.cpuFreq.textContent = data.cpuFreqMHz ? `${data.cpuFreqMHz} MHz` : "Unknown"
    elements.flashSize.textContent = data.flashChipSize ? formatBytes(data.flashChipSize) : "Unknown"

    // Update status
    elements.updateStatus.textContent = data.status || "Idle"
    elements.updateStatus.className = "status-badge"

    if (data.status && data.status.toLowerCase().includes("error")) {
      elements.updateStatus.classList.add("status-error")
      state.updateInProgress = false
      enableButtons()
    } else if (data.otaInProgress) {
      elements.updateStatus.classList.add("status-updating")
      updateProgressBar(data.progress || 0, data.status || "Updating...")
      state.updateInProgress = true
    } else if (data.status && data.status.toLowerCase().includes("success")) {
      elements.updateStatus.classList.add("status-success")
      state.updateInProgress = false
    } else {
      elements.updateStatus.classList.add("status-idle")
      state.updateInProgress = false
    }
  }

  function handleFileSelect() {
    if (elements.firmwareFile.files.length > 0) {
      const file = elements.firmwareFile.files[0]
      elements.fileName.textContent = file.name

      // Check file size and warn if it's large
      if (file.size > 1000000) {
        // 1MB
        const sizeMB = (file.size / 1048576).toFixed(2)
        showModal(
          "Large File Warning",
          `The selected firmware file is ${sizeMB}MB. Large files may take longer to upload and could cause timeout issues.`,
          null,
        )
      }
    } else {
      elements.fileName.textContent = "No file selected"
    }
  }

  function handleUpdateSubmit(e) {
    e.preventDefault()

    if (state.updateInProgress) {
      showModal("Update in Progress", "Please wait for the current update to complete.")
      return
    }

    if (!elements.firmwareFile.files.length) {
      showModal("No File Selected", "Please select a firmware binary file (.bin) to upload.")
      return
    }

    showModal(
      "Confirm Update",
      "Are you sure you want to update the firmware? The device will restart after the update.",
      startUpdate,
    )
  }

  function confirmRestart() {
    showModal("Confirm Restart", "Are you sure you want to restart the device?", restartDevice)
  }

  function startUpdate() {
    if (!elements.firmwareFile.files.length) return

    const file = elements.firmwareFile.files[0]

    // Check file type
    if (!file.name.endsWith(".bin")) {
      showModal("Invalid File", "Please select a valid firmware binary (.bin) file.")
      return
    }

    // Prepare form data
    const formData = new FormData()
    formData.append("update", file)

    state.updateInProgress = true
    updateProgressBar(0, "Starting update...")
    disableButtons()

    // Set timeout based on file size
    const timeoutSeconds = Math.max(120, 30 + Math.ceil(file.size / 1048576) * 10)
    const timeoutMs = timeoutSeconds * 1000

    console.log(`Setting timeout to ${timeoutSeconds} seconds for ${formatBytes(file.size)} file`)

    const controller = new AbortController()
    const timeoutId = setTimeout(() => controller.abort(), timeoutMs)

    // Show message for large files
    if (file.size > 1000000) {
      const sizeMB = (file.size / 1048576).toFixed(2)
      updateProgressBar(0, `Uploading large file (${sizeMB}MB). Please be patient...`)
    }

    // Use fetch with streaming for better performance
    fetch("/api/update", {
      method: "POST",
      body: formData,
      signal: controller.signal,
    })
      .then((response) => {
        clearTimeout(timeoutId)
        if (!response.ok) {
          throw new Error(`Update failed: ${response.status} ${response.statusText}`)
        }
        return response.text()
      })
      .then((data) => {
        console.log("Update response:", data)
        handleUpdateSuccess()
      })
      .catch((error) => {
        console.error("Error during update:", error)
        handleUpdateError(error)
      })

    // Set up polling for progress updates (less frequent to reduce load)
    const progressInterval = setInterval(() => {
      if (!state.updateInProgress) {
        clearInterval(progressInterval)
        return
      }

      fetch("/api/info", { timeout: 3000 })
        .then((response) => response.json())
        .then((data) => updateProgressFromData(data, progressInterval))
        .catch((error) => console.error("Error fetching update progress:", error))
    }, 3000) // Check every 3 seconds instead of 2
  }

  function updateProgressFromData(data, progressInterval) {
    if (data.otaInProgress) {
      updateProgressBar(data.progress || 0, data.status || "Updating...")
    } else if (data.status && data.status.includes("successful")) {
      updateProgressBar(100, "Update successful! Rebooting...")
      clearInterval(progressInterval)
    } else if (data.status && data.status.includes("Error")) {
      updateProgressBar(0, data.status || "Update failed")
      state.updateInProgress = false
      clearInterval(progressInterval)
      enableButtons()
    }
  }

  function handleUpdateSuccess() {
    updateProgressBar(100, "Update successful! Rebooting...")
    showModal(
      "Update Successful",
      "The firmware has been updated successfully. The device is rebooting. This page will reload when the device is back online.",
    )

    // Start checking for device to come back online
    startReconnectionCheck()
  }

  function handleUpdateError(error) {
    // Don't immediately set updateInProgress to false
    // The device might still be processing the update
    updateProgressBar(0, "Update request failed, but device may still be updating...")

    // Start checking if the device is still accessible
    startReconnectionCheck()

    if (error.name === "AbortError") {
      showModal(
        "Update Timeout",
        "The update request timed out. For large firmware files, the device may still be processing the update. Please wait a few minutes before trying to reconnect.",
      )
    } else {
      showModal(
        "Update Error",
        `There was an error uploading the firmware: ${error.message}. The device may still be updating or may have rebooted.`,
      )
    }
  }

  function restartDevice() {
    fetch("/api/restart", {
      method: "POST",
    })
      .then((response) => {
        if (!response.ok) throw new Error("Restart failed")
        return response.text()
      })
      .then(() => {
        showModal(
          "Device Restarting",
          "The device is restarting. This page will reload when the device is back online.",
        )

        // Start checking for device to come back online
        startReconnectionCheck()
      })
      .catch((error) => {
        console.error("Error restarting device:", error)
        showModal("Restart Failed", "There was an error restarting the device. Please try again.")
      })
  }

  function startReconnectionCheck() {
    state.reconnectAttempts = 0
    if (!state.updateCheckInterval) {
      state.updateCheckInterval = setInterval(() => {
        checkConnection().then((connected) => {
          if (connected) {
            clearInterval(state.updateCheckInterval)
            state.updateCheckInterval = null
            window.location.reload()
          } else {
            state.reconnectAttempts++
            if (state.reconnectAttempts >= state.MAX_RECONNECT_ATTEMPTS) {
              clearInterval(state.updateCheckInterval)
              state.updateCheckInterval = null

              // Now we can safely assume the update failed
              state.updateInProgress = false
              enableButtons()

              showModal(
                "Connection Lost",
                "Could not reconnect to the device. Please check if the device is online and refresh the page.",
                () => window.location.reload(),
              )
            }
          }
        })
      }, 10000) // Check every 10 seconds
    }
  }

  function handleConnectionError() {
    // If we're in the middle of an update and can't connect, the device might be rebooting
    if (state.updateInProgress) {
      elements.progressText.textContent = "Device is rebooting or updating..."
      startReconnectionCheck()
    }
  }

  // Utility functions
  function checkConnection() {
    return fetch("/api/info", {
      method: "HEAD", // Use HEAD request for faster checking
      cache: "no-store",
      timeout: 5000,
    })
      .then(() => true)
      .catch(() => false)
  }

  function updateProgressBar(progress, status) {
    elements.progressBarFill.style.width = `${progress}%`
    elements.progressText.textContent = status
    elements.progressPercentage.textContent = `${progress}%`
  }

  function showModal(title, body, confirmCallback) {
    elements.modalTitle.textContent = title
    elements.modalBody.textContent = body

    if (confirmCallback) {
      elements.modalConfirm.style.display = "block"
      elements.modalConfirm.onclick = () => {
        confirmCallback()
        closeModal()
      }
    } else {
      elements.modalConfirm.style.display = "none"
    }

    elements.modal.style.display = "flex"
  }

  function closeModal() {
    elements.modal.style.display = "none"
  }

  function formatBytes(bytes, decimals = 2) {
    if (bytes === 0) return "0 Bytes"
    const k = 1024
    const dm = decimals < 0 ? 0 : decimals
    const sizes = ["Bytes", "KB", "MB", "GB", "TB"]
    const i = Math.floor(Math.log(bytes) / Math.log(k))
    return Number.parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + " " + sizes[i]
  }

  function formatUptime(uptime) {
    const days = Math.floor(uptime / 86400)
    const hours = Math.floor((uptime % 86400) / 3600)
    const minutes = Math.floor((uptime % 3600) / 60)
    const seconds = uptime % 60

    let uptimeText = ""
    if (days > 0) uptimeText += `${days}d `
    if (hours > 0 || days > 0) uptimeText += `${hours}h `
    if (minutes > 0 || hours > 0 || days > 0) uptimeText += `${minutes}m `
    uptimeText += `${seconds}s`
    return uptimeText
  }

  function disableButtons() {
    elements.updateBtn.disabled = true
    elements.restartBtn.disabled = true
  }

  function enableButtons() {
    elements.updateBtn.disabled = false
    elements.restartBtn.disabled = false
  }
})

