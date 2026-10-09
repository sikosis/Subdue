import AppKit
import Foundation

struct AppTarget: Identifiable, Hashable {
    let id: String
    let name: String
    let bundleIdentifier: String?
    let bundleURL: URL?
    let processIdentifier: pid_t?

    init(name: String, bundleIdentifier: String? = nil, bundleURL: URL? = nil, processIdentifier: pid_t? = nil) {
        self.name = name
        self.bundleIdentifier = bundleIdentifier
        self.bundleURL = bundleURL
        self.processIdentifier = processIdentifier
        self.id = bundleIdentifier ?? bundleURL?.path ?? name
    }

    init(application: NSRunningApplication) {
        self.init(
            name: application.localizedName ?? application.bundleIdentifier ?? "Unknown application",
            bundleIdentifier: application.bundleIdentifier,
            bundleURL: application.bundleURL,
            processIdentifier: application.processIdentifier
        )
    }

    var detail: String {
        bundleIdentifier ?? bundleURL?.path ?? "Application name"
    }

    func matchingRunningApplications() -> [NSRunningApplication] {
        let workspace = NSWorkspace.shared.runningApplications
        if let processIdentifier,
           let exact = NSRunningApplication(processIdentifier: processIdentifier),
           !exact.isTerminated {
            return [exact]
        }
        if let bundleIdentifier {
            let matches = NSRunningApplication.runningApplications(withBundleIdentifier: bundleIdentifier)
            if !matches.isEmpty { return matches }
        }
        return workspace.filter {
            $0.localizedName?.localizedCaseInsensitiveCompare(name) == .orderedSame ||
            $0.bundleURL?.deletingPathExtension().lastPathComponent.localizedCaseInsensitiveCompare(name) == .orderedSame
        }
    }
}
