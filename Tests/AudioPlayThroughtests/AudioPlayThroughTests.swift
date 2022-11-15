//
//  File.swift
//  
//
//  Created by Devin Roth on 2022-06-20.
//

import Foundation
import AVFoundation
import CoreAudio
import AudioToolbox
import XCTest

import AudioPlayThroughObjC




final class TranslateUIDToDeviceTests: XCTestCase {
    
    
    
    func testExample() throws {
        
        let audioPlayThrough = AudioPlayThroughObjC()

        XCTAssert(noErr == audioPlayThrough.create(
            "BuiltInSpeakerDevice" as CFString,
            "BuiltInMicrophoneDevice" as CFString
        ))
        
        audioPlayThrough.setPeakCallback { peak in
            print(peak)
        }
        
        XCTAssert(noErr == audioPlayThrough.start())
        
        sleep(10)
        
        XCTAssert(noErr == audioPlayThrough.stop())
        
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct
        // results.
        
        
    }
}
