# Feature Specification: Weather, Date/Time & Bitcoin Info Display

**Feature Branch**: `001-weather-datetime-btc`  
**Created**: 2025-02-24  
**Status**: Draft  
**Input**: User description: "This application will download weather, and date time information, and bitcoin price from internet, and display to users."

## Clarifications

### Session 2025-02-24

- Q: How is the weather location determined? → A: Single fixed location (e.g. city name or coordinates) set at setup (config or compile-time).
- Q: Is the refresh interval fixed or user-configurable? → A: Fixed interval only (e.g. 15 minutes) for this version.
- Q: Is bitcoin price shown in USD only or user-configurable currency? → A: USD only for this version.
- Q: What should the display show before the first successful data fetch? → A: Show "Loading..." or similar placeholder until first successful fetch.
- Q: Which temperature unit for weather (C or F)? → A: User-configurable (C or F).

### Session 2025-02-25

- Q: Which Logseq API/integration should be used to read today's to-do list? → A: Logseq Graph API (official HTTP API for querying the graph).
- Q: How should the device authenticate to the Logseq server? → A: Always require API key or token in config (server must be protected).
- Q: What should the display show for each to-do item? → A: Title and status (done vs pending), e.g. checkbox or label.
- Q: How often should the to-do list refresh? → A: Same fixed interval as date/time, weather, and bitcoin (e.g. 15 minutes).
- Q: Where should the Logseq server address and API key be configured? → A: Compile-time only (e.g. build flags or platformio.ini env); change requires recompile.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - See current date and time (Priority: P1)

A user looks at the display and sees the current date and time so they can know the date and time at a glance without using another device.

**Why this priority**: Date and time are the most universal and frequently needed information; the display is usable as a clock even if other data is missing.

**Independent Test**: Can be fully tested by ensuring the display shows a recognizable date and time that matches the current moment (within one minute), and delivers immediate value as a clock.

**Acceptance Scenarios**:

1. **Given** the device is powered and connected to the internet, **When** the user looks at the display, **Then** the current date is visible (e.g. day/month/year or equivalent).
2. **Given** the device is powered and connected to the internet, **When** the user looks at the display, **Then** the current time is visible and updates to reflect the passage of time (e.g. at next refresh).
3. **Given** the device has previously shown date and time, **When** the device has no internet access, **Then** the last known date and time remain visible or an offline indicator is shown.

---

### User Story 2 - See current weather (Priority: P2)

A user looks at the display and sees current weather information (e.g. condition and temperature) so they can decide how to dress or plan their day without opening another app.

**Why this priority**: Weather is high-value, frequently checked information that makes the display useful for daily planning.

**Independent Test**: Can be fully tested by verifying that the display shows weather condition and temperature that match a trusted source for the same location and time.

**Acceptance Scenarios**:

1. **Given** the device is powered and connected to the internet, **When** the user looks at the display, **Then** current weather condition (e.g. sunny, cloudy, rain) is visible.
2. **Given** the device is powered and connected to the internet, **When** the user looks at the display, **Then** current temperature (or feels-like) is visible in the user-selected unit (C or F).
3. **Given** the device has previously shown weather, **When** the device has no internet access, **Then** the last known weather is shown or an offline indicator is shown.

---

### User Story 3 - See current Bitcoin price (Priority: P3)

A user looks at the display and sees the current bitcoin price so they can track market value at a glance without opening a trading or finance app.

**Why this priority**: Bitcoin price completes the requested set of data and adds clear value for users interested in that metric.

**Independent Test**: Can be fully tested by comparing the displayed price to a trusted public source at the same moment (within one refresh cycle).

**Acceptance Scenarios**:

1. **Given** the device is powered and connected to the internet, **When** the user looks at the display, **Then** the current bitcoin price is visible in USD.
2. **Given** the device has previously shown bitcoin price, **When** the device has no internet access, **Then** the last known price is shown or an offline indicator is shown.

---

### User Story 4 - Information stays current (Priority: P4)

A user leaves the display running and expects the information to refresh periodically so that date/time, weather, and bitcoin price do not become stale.

**Why this priority**: Periodic refresh is necessary for the display to remain useful over time.

**Independent Test**: Can be fully tested by leaving the device running and confirming that displayed values change at the expected interval (e.g. time advances, weather/price can change).

**Acceptance Scenarios**:

1. **Given** the device is running and connected, **When** the fixed refresh interval (e.g. 15 minutes) has elapsed, **Then** the display updates with fresh data (or clearly indicates an update attempt).
2. **Given** the device is running, **When** an update fails (e.g. network error), **Then** the display retains the last good data or shows an offline/error state and retries later.

---

### User Story 5 - See Logseq today to-do list (Priority: P5)

A user looks at the display and sees today's to-do list from their Logseq server (each item with title and done/pending status) so they can track daily tasks at a glance without opening Logseq.

**Why this priority**: Logseq to-do adds a fourth data domain; server URL and API token are configured at compile-time; same refresh interval as other data.

**Independent Test**: With WiFi, valid LOGSEQ_SERVER_URL and LOGSEQ_API_TOKEN, and Logseq server reachable with today's journal containing TODO/DONE blocks, the display shows to-do items with title and status; without config or on 401/unreachable, shows last list or "Unavailable" for the to-do section.

**Acceptance Scenarios**:

1. **Given** the device is powered, connected to the internet, and configured with Logseq server URL and API token, **When** the user looks at the display, **Then** today's to-do list from Logseq is visible with each item's title and status (done vs pending).
2. **Given** the device has previously shown the Logseq to-do list, **When** the Logseq server is unreachable or returns 401, **Then** the last successfully loaded to-do list is shown or an offline/unavailable indicator is shown for the to-do section.

---

### Edge Cases

- What does the user see before the first successful data fetch? The display shows a "Loading..." or similar placeholder until the first successful fetch.
- What happens when the device has no internet on first run? The display shows an offline or “no data” state until the first successful fetch.
- What happens when a data source returns invalid or missing data? The display shows the last valid value for that item, or a clear “unavailable” for that section, without breaking the rest of the display.
- What happens when the user is in a different timezone than the data source? Date and time are shown in a consistent, user-relevant timezone (e.g. configurable or device local).
- What happens when price or weather APIs are temporarily unavailable? The system retries on the next refresh and continues to show last known data until success.
- What happens when the Logseq server is unreachable, returns 401, or today's page has no to-dos? The display retains the last successfully loaded to-do list or shows an offline/unavailable indicator for the to-do section; the system retries on the next refresh (same as other data sources).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST display current date and time to the user in a readable format.
- **FR-002**: System MUST display current weather information to the user, including at least condition and temperature in the user-selected unit (Celsius or Fahrenheit).
- **FR-003**: System MUST display current bitcoin price to the user in USD only.
- **FR-004**: System MUST obtain weather, date/time, and bitcoin data from the internet.
- **FR-005**: System MUST refresh the displayed data (including the Logseq today to-do list) at a fixed interval (e.g. 15 minutes) so information stays current.
- **FR-006**: System MUST handle absence of internet by showing last available data or a clear offline/unavailable indicator for each affected section.
- **FR-007**: System MUST present all information in a readable layout on the display so that date/time, weather, bitcoin price, and Logseq to-do (when configured) are distinguishable.
- **FR-008**: System MUST tolerate temporary failures of one or more data sources without preventing the display of other data that is available.
- **FR-009**: Before the first successful data fetch, system MUST display a loading or placeholder state (e.g. "Loading...").
- **FR-010**: System MUST read today's to-do list from a Logseq server via the Logseq Graph API (official HTTP API); the server address SHALL be configurable at compile-time (e.g. build flags or platformio.ini).
- **FR-011**: System MUST authenticate to the Logseq server using an API key or token; the key/token SHALL be configurable at compile-time (required for this feature).
- **FR-012**: System MUST display each today's to-do item with its title and status (done vs pending), e.g. via checkbox or label.

### Key Entities

- **Weather**: Represents current conditions for a single fixed location set at setup; key attributes include condition description and temperature in user-configurable unit (C or F).
- **Date/Time**: Represents the current moment in a user-relevant timezone; key attributes include date and time.
- **Bitcoin price**: Represents the current market price of bitcoin in USD; key attribute is numeric value in USD.
- **Display state**: What is shown to the user; may include last successful values and an offline/error state per section.
- **Logseq today to-do item**: Represents a single task on the Logseq "today" page; key attributes are title (text) and status (done vs pending), displayed e.g. with checkbox or label.

## Assumptions

- The display device is a fixed-purpose screen (e.g. e-paper or similar) used in a home or office. Information is presented in a readable format and layout (legible at typical viewing distance on the 4.7" e-paper).
- Weather is shown for a single fixed location (e.g. city name or coordinates) set at setup (config or compile-time).
- Date/time is shown in one timezone (device local or configurable).
- Bitcoin price is shown in USD only.
- Temperature is shown in user-configurable units (Celsius or Fahrenheit).
- Refresh interval is fixed for this version (e.g. 15 minutes); not user-configurable.
- Data sources are public or freely usable; no user authentication is required to fetch data for this feature.
- Logseq today's to-do list is read via the official Logseq Graph API (HTTP); the Logseq server base URL and API key/token are configurable at compile-time only (e.g. build flags or platformio.ini); the server is assumed to be protected and authentication is required. To-do list refreshes at the same fixed interval as other data (e.g. 15 minutes).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: User can see current date and time on the display within one minute of the device having internet access.
- **SC-002**: User can see current weather (condition and temperature), bitcoin price, and (when configured) Logseq today to-do list on the display when the device has internet access and data sources are responding.
- **SC-003**: Under normal conditions, displayed data updates at least every 15 minutes (fixed interval), so information does not become stale for typical use.
- **SC-004**: When the device has no internet access, the user sees either the last successfully loaded data or a clear offline/unavailable indication within one refresh cycle.
- **SC-005**: Users can distinguish date/time, weather, bitcoin price, and (when configured) Logseq to-do list on the display without confusion.
