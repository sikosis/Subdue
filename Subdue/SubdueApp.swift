import AppKit
import SwiftUI

@main
struct SubdueApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate
    @StateObject private var model = SchedulerModel()

    var body: some Scene {
        WindowGroup(id: "main") {
            ContentView()
                .environmentObject(model)
                .onAppear { appDelegate.configure(model: model) }
        }
        .windowResizability(.contentSize)
        .commands {
            CommandGroup(replacing: .newItem) { }
        }

        MenuBarExtra {
            MenuBarView()
                .environmentObject(model)
        } label: {
            MenuBarLabel()
                .environmentObject(model)
        }
        .menuBarExtraStyle(.menu)
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    private let options = CommandLineOptions.parse(Array(CommandLine.arguments.dropFirst()))
    private var configured = false

    func applicationWillFinishLaunching(_ notification: Notification) {
        if options.showHelp || options.error != nil {
            let output = options.error.map { "Error: \($0)\n\n" } ?? ""
            FileHandle.standardOutput.write(Data((output + CommandLineOptions.usage + "\n").utf8))
        }
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        if options.showHelp || options.error != nil {
            NSApplication.shared.terminate(nil)
        }
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        if !flag {
            sender.windows.first(where: { $0.title == "Subdue" })?.makeKeyAndOrderFront(nil)
        }
        sender.activate(ignoringOtherApps: true)
        return true
    }

    @MainActor
    func configure(model: SchedulerModel) {
        guard !configured else { return }
        configured = true
        guard let schedule = options.schedule else { return }
        if options.muteAudio {
            model.armMute(choice: schedule)
        } else if let app = options.application {
            model.arm(target: model.target(for: app), choice: schedule, forceIfNeeded: options.forceIfNeeded)
        } else {
            return
        }
        if !options.foreground {
            NSApplication.shared.windows.first?.orderOut(nil)
        }
    }
}
