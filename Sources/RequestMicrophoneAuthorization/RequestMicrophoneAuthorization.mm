//
//  RequestMicrophoneAuthorization.m
//  AudioPassThrough
//
//  Created by Devin Roth on 2020-07-14.
//  Copyright © 2020 Existential Audio. All rights reserved.
//

#import "RequestMicrophoneAuthorization.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <syslog.h>

void RequestMicrophoneAuthorization()
{
    // Request permission to access the camera and microphone.
    if (@available(macOS 10.14, *)) {
        switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio])
        {
            case AVAuthorizationStatusAuthorized:
            {
                // The user has previously granted access to the camera.
                break;
            }
            case AVAuthorizationStatusNotDetermined:
            {
                // The app hasn't yet asked the user for camera access.
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                    if (granted) {
                    }
                }];
                break;
            }
            case AVAuthorizationStatusDenied:
            {
                // The user has previously denied access.
                return;
            }
            case AVAuthorizationStatusRestricted:
            {
                // The user can't grant access due to restrictions.
                return;
            }
        }
    } else {
        // Fallback on earlier versions
    }
}
