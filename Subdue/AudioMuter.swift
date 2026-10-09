import CoreAudio
import Foundation

enum AudioMuter {
    static func muteDefaultOutput() -> Bool {
        var deviceID = AudioDeviceID(kAudioObjectUnknown)
        var deviceSize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var defaultDeviceAddress = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )

        guard AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &defaultDeviceAddress,
            0,
            nil,
            &deviceSize,
            &deviceID
        ) == noErr, deviceID != kAudioObjectUnknown else {
            return false
        }

        var muteAddress = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyMute,
            mScope: kAudioDevicePropertyScopeOutput,
            mElement: kAudioObjectPropertyElementMain
        )
        var isSettable = DarwinBoolean(false)
        if AudioObjectHasProperty(deviceID, &muteAddress),
           AudioObjectIsPropertySettable(deviceID, &muteAddress, &isSettable) == noErr,
           isSettable.boolValue {
            var muted: UInt32 = 1
            if AudioObjectSetPropertyData(
                deviceID,
                &muteAddress,
                0,
                nil,
                UInt32(MemoryLayout<UInt32>.size),
                &muted
            ) == noErr {
                return true
            }
        }

        var volumeAddress = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyVolumeScalar,
            mScope: kAudioDevicePropertyScopeOutput,
            mElement: kAudioObjectPropertyElementMain
        )
        isSettable = DarwinBoolean(false)
        guard AudioObjectHasProperty(deviceID, &volumeAddress),
              AudioObjectIsPropertySettable(deviceID, &volumeAddress, &isSettable) == noErr,
              isSettable.boolValue else {
            return false
        }

        var silence: Float32 = 0
        return AudioObjectSetPropertyData(
            deviceID,
            &volumeAddress,
            0,
            nil,
            UInt32(MemoryLayout<Float32>.size),
            &silence
        ) == noErr
    }
}
