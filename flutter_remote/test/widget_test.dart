import 'package:flutter_test/flutter_test.dart';
import 'package:esp32_music_remote/main.dart';

void main() {
  testWidgets('Croenix Music Player App initial smoke test', (WidgetTester tester) async {
    await tester.pumpWidget(const CroenixMusicPlayerApp());
    expect(find.text('CROENIX AUDIO'), findsOneWidget);
    expect(find.text('Now Playing'), findsOneWidget);
    expect(find.text('SD Card Files'), findsOneWidget);
  });
}
