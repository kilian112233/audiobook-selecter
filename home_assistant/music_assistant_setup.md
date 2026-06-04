# Music Assistant Setup for Audiobooks

## Quick Start

### 1. Create Playlists/Collections in Music Assistant
Your audiobooks should be organized in Music Assistant. You can:

**Option A: Create Playlists** (Easiest)
1. Go to Music Assistant in Home Assistant
2. Create a new playlist for each audiobook
3. Add all the audio files for that audiobook to the playlist
4. Use the playlist name in the dashboard buttons

**Option B: Use Folders** (If your audiobooks are in folders)
1. Keep audiobook files in separate folders
2. Music Assistant will recognize them as collections
3. Reference the folder name in dashboard buttons

### 2. Configure Home Assistant Media Player
Your `media_player.bedroom` should be:
- A speaker/media device (ALSA output, AirPlay, Casting, etc.)
- Available in Home Assistant
- Connected to your bedroom audio system

### 3. Update Dashboard with Your Audiobooks
Edit `home_assistant/audiobook_selector_lovelace.yaml`:

Replace:
```yaml
media_content_id: "media-source://music_assistant/playlists/Audiobook 1"
```

With your actual audiobook names. To find them:
1. Go to Home Assistant Media Browser
2. Navigate to Music Assistant
3. Find your audiobook playlists
4. Note the exact names

### 4. How Your Dad Selects Audiobooks

**Method 1: Dashboard Buttons (Easiest)**
- Open Home Assistant on his phone/tablet/web browser
- Go to the "🎧 Audiobook Selector" view
- Tap the audiobook button to start playing
- Use the bedside ESP32 buttons to pause/rewind/forward

**Method 2: Voice Control**
- "Alexa, play [Audiobook Name] on the bedroom speaker"
- (If using Alexa integration)

**Method 3: Home Assistant App Shortcuts**
- Create quick access shortcuts in the Home Assistant app
- One tap to play each audiobook

## Example: Setting Up an Audiobook in Music Assistant

1. Go to Music Assistant → Add Library
2. Select your audiobook source (Local Files, Spotify, Audible, etc.)
3. Create a Playlist: "Audiobook - The Hobbit"
4. Add all chapter files to the playlist
5. In the dashboard YAML, add:
   ```yaml
   - type: custom:button-card
     name: 📚 The Hobbit
     tap_action:
       action: call-service
       service: media_player.play_media
       service_data:
         entity_id: media_player.bedroom
         media_content_type: playlist
         media_content_id: "media-source://music_assistant/playlists/The Hobbit"
   ```

## Troubleshooting

### "media_player.bedroom" not found
- Check your media player entity name in Home Assistant
- Go to: Settings → Devices & Services → Entities
- Search for your speaker, note the exact entity ID
- Update all references in the config

### Buttons don't work
- Verify audiobook playlist name matches exactly
- Check Music Assistant can see the playlist
- Check bedroom media player is online/available

### Audio not playing
- Verify bedroom speaker is selected as output in Music Assistant
- Check speaker volume
- Verify media player is powered on
